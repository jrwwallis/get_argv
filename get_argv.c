#define _GNU_SOURCE
#include "get_argv.h"

#include <errno.h>
#include <stdint.h>
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

static int is_stack_growth_down(int *caller_stack_var) {
  int callee_stack_var = 0;
  return (intptr_t)&callee_stack_var < (intptr_t)caller_stack_var;
}

int get_argv(const char *const **const argv, int *const argc) {
  int i;

  /* Ensure Linux downwards stack growth */
  int caller_stack_var = 0;
  if (!is_stack_growth_down(&caller_stack_var)) {
    errno = ENOEXEC;
    return -1;
  }

  const long sc_ret = sysconf(_SC_PAGESIZE);
  if (sc_ret < 1) {
    return -1;
  }
  const size_t page_size = sc_ret;

  /* walk the environment pointer array to find the last entry */
  for (i = 0; environ[i] != NULL; ++i) {
  }
  const char *const last_env = environ[i - 1];

  const size_t last_env_len = strlen(last_env);

  /* program name comes after environ strings */
  const char *prog_name = last_env + last_env_len + 1;
  const size_t prog_name_len = strlen(prog_name);

  /* stack end should be very close to end of the program name string */
  const char *prog_name_end = prog_name + prog_name_len;
  /* round up to the end of the page */
  const uintptr_t stack_end =
      ((uintptr_t)prog_name_end & ~(page_size - 1)) + page_size;

  struct rlimit rlim;
  if (getrlimit(RLIMIT_STACK, &rlim) != 0) {
    return -1;
  }
  const off_t stack_size = rlim.rlim_cur;
  const void *stack_start = (const void *)(stack_end - stack_size);

  /* represent the whole stack as an array of pointers */
  const void *const *const stack_ptr_arr = (void *)stack_start;
  const size_t stack_ptr_arr_sz = (stack_size / sizeof(void *));

  /* search backwards through the stack array until environ pointer is found */
  for (i = stack_ptr_arr_sz - 1; i >= 0; --i) {
    if (stack_ptr_arr[i] == environ) {
      break;
    }
  }
  if (i < 0) {
    errno = ENOEXEC;
    return -1;
  }

  /* argc and argv immediately precede the environ pointer */
  *argv = stack_ptr_arr[i - 1];
  *argc = (uintptr_t)stack_ptr_arr[i - 2];

  return 0;
}
