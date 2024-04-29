#define _GNU_SOURCE
#include "get_argv.h"

#if defined(__APPLE__) && defined(__MACH__)
#include <crt_externs.h>
#endif /* __APPLE__ && __MACH__ */
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
// clang-format off
/*
** ELF Stack Layout **
(Credit: https://www.win.tue.nl/~aeb/linux/hh/stack-layout.html)
(Authoritative source: https://gitlab.com/x86-psABIs/x86-64-ABI)

lower addresses:
    function stack frames                          <- 2) Lower search bound in current stack
    ...
    local variables of main
    saved registers of main
    return address of main
    argc                                           <- 5) argc immediately precedes argv
    argv (char ** pointer)                         <- 4) argv immediately precedes envp
    envp (char ** pointer)                         <- 3) Search for this pointer
    stack from startup code
    argc
    argv pointers (char * pointer array)
    argv[] terminating NULL
    environment pointers (char * pointer array)    <- 1) Upper search bound
    envp[] terminating NULL
    ELF Auxiliary Table
    argv strings
    environment strings
    program name
    NULL
higher addresses:

 */
// clang-format on

/* get_argv() strongly depends on how ELF lays out the stack and is unlikely
 * to work in any other executable format.  This reference to the ELF-specific
 * init_array section should not compile if ELF is not being used */
extern void *__init_array_start;
static void *is_elf __attribute__((unused)) = &__init_array_start;
#if defined(__APPLE__) && defined(__MACH__)
extern char **environ;
#endif /* __APPLE__ && __MACH__ */

static const int is_stack_growth_down(const int *const caller_stack_var) {
  const int callee_stack_var = 0;
  return (const uintptr_t)&callee_stack_var < (const uintptr_t)caller_stack_var;
}

int get_argv(const char *const **const argv, int *const argc) {
#if defined(__APPLE__) && defined(__MACH__)
  *argv = *(const char *const **const)_NSGetArgv();
  *argc = *_NSGetArgc();
  return 0;
#endif /* __APPLE__ && __MACH__ */

  int i;

  /* Ensure Linux downwards stack growth */
  const int caller_stack_var = 0;
  if (!is_stack_growth_down(&caller_stack_var)) {
    errno = ENOEXEC;
    return -1;
  }

#define ALIGN_PTR(ptr, align_type) (typeof(ptr))((uintptr_t)(ptr) & ~(sizeof(align_type) -1));

  /* Derive upper and lower bounds for a safe range of stack to search through
   * for the environ pointer */
  const void *stack_upper_bound = ALIGN_PTR(environ[0], void *);
  const void *stack_lower_bound = ALIGN_PTR(&stack_upper_bound, void *);

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
#if defined __x86_64__
  *argv = stack_ptr_arr[i - 1];
  *argc = (uintptr_t)stack_ptr_arr[i - 2];
#elif defined __aarch64__
  *argv = stack_ptr_arr[i - 7];
  *argc = (uintptr_t)stack_ptr_arr[i - 6];
#else
#error Unknown architecture
#endif

  return 0;
}
