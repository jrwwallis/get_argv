#define _GNU_SOURCE
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

/*
ELF stack layout:
(Credit: https://www.win.tue.nl/~aeb/linux/hh/stack-layout.html)

stack_start:
    ...
    local variables of main
    saved registers of main
    return address of main
    argc
    argv
    envp
    stack from startup code
    argc
    argv pointers
    NULL that ends argv[]
    environment pointers
    NULL that ends envp[]
    ELF Auxiliary Table
    argv strings
    environment strings
    program name
    NULL
stack_end:

*/

/* get_argv() strongly depends on how ELF lays out the stack and is unlikely
 * to work in any other executable format.  This reference the ELF-specific
 * init_array section should not compile if ELF is not being used */
extern void *__init_array_start;
static void *is_elf __attribute__((unused)) = &__init_array_start;

int get_argv(char ***argv, int *argc) {
    int i;

    long sc_ret = sysconf(_SC_PAGESIZE);
    if (sc_ret < 1) {
        return -1;
    }
    size_t page_size = sc_ret;

    /* walk the environment pointer array to find the last entry */
    for (i = 0; environ[i] != NULL; ++i) { }
    char *last_env = environ[i - 1];

    size_t last_env_len = strlen(last_env);

    /* program name comes after environ strings */
    char *prog_name = last_env + last_env_len + 1;
    size_t prog_name_len = strlen(prog_name);

    /* stack end should be very close to end of the program name string */
    uintptr_t stack_end = (uintptr_t)prog_name + prog_name_len;

    /* round up to the end of the page */
    stack_end = (stack_end & ~(page_size - 1)) + page_size;

    struct rlimit rlim;
    getrlimit(RLIMIT_STACK, &rlim);
    off_t stack_size = rlim.rlim_cur;
    uintptr_t stack_start = stack_end - stack_size;

    /* represent the whole stack as an array of pointers */
    void ** stack_ptr_arr = (void *)stack_start;
    size_t stack_ptr_arr_sz = (stack_size / sizeof(void *)) - 1;

    /* search backwards through the stack array until environ pointer is found */
    for (i = stack_ptr_arr_sz; i >= 0; --i) {
        if (stack_ptr_arr[i] == environ) {
            break;
        }
    }
    if (i < 0) {
        errno = EINVAL;
        return -1;
    }

    /* argc and argv immediately precede the environ pointer */
    *argv = stack_ptr_arr[i - 1];
    *argc = (uintptr_t)stack_ptr_arr[i - 2];

    return 0;
}

int main (int argc, char **argv) {
    char **local_argv;
    int local_argc;
    int i;

    printf ("argc=%d, argv=%p\n", argc, argv);
    for (i = 0; i < argc; ++i) {
        printf("argv[%d]=%s\n", i, argv[i]);
    }

    get_argv(&local_argv, &local_argc);

    printf ("argc=%d, argv=%p\n", local_argc, local_argv);
    for (i = 0; i < local_argc; ++i) {
        printf("argv[%d]=%s\n", i, local_argv[i]);
    }

    return 0;
}