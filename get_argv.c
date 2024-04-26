#define _GNU_SOURCE
#include "get_argv.h"

#include <errno.h>
#include <stdint.h>
#include <unistd.h>

/*
** ELF Stack Layout **
(Credit: https://www.win.tue.nl/~aeb/linux/hh/stack-layout.html)
(Authoritative source: https://gitlab.com/x86-psABIs/x86-64-ABI)

lower addresses:
    function stack frames                          <- Lower search bound
    ...
    local variables of main
    saved registers of main
    return address of main
    argc                                           <- argc and argv immediately
    argv (char ** pointer)                         <-   precede envp
    envp (char ** pointer)                         <- Search for this pointer
    stack from startup code
    argc
    argv pointers (char * pointer array)
    argv[] terminating NULL                        <- Upper search bound
    environment pointers (char * pointer array)
    envp[] terminating NULL
    ELF Auxiliary Table
    argv strings
    environment strings
    program name
    NULL
higher addresses:

*/

/* get_argv() strongly depends on how ELF lays out the stack and is unlikely
 * to work in any other executable format.  This reference to the ELF-specific
 * init_array section should not compile if ELF is not being used */
extern void *__init_array_start;
static void *is_elf __attribute__((unused)) = &__init_array_start;

static const int is_stack_growth_down(const int *const caller_stack_var) {
  const int callee_stack_var = 0;
  return (const uintptr_t)&callee_stack_var < (const uintptr_t)caller_stack_var;
}

int get_argv(const char *const **const argv, int *const argc) {
  int i;

  /* Ensure Linux downwards stack growth */
  const int caller_stack_var = 0;
  if (!is_stack_growth_down(&caller_stack_var)) {
    errno = ENOEXEC;
    return -1;
  }

  /* Derive upper and lower bounds for a safe range of stack to search through
   * for the environ pointer */
  const void *stack_upper_bound = environ[0];
  const void *stack_lower_bound = &stack_upper_bound;

  /* Represent the bounded range of stack as an array of pointers */
  const void *const *const stack_ptr_arr = (void *)stack_lower_bound;
  const size_t stack_ptr_arr_sz =
      (stack_upper_bound - stack_lower_bound) / sizeof(void *);

  /* Search backwards through the stack array until environ pointer is found */
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
