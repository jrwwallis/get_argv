#include "get_argv.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

int main(const int argc, const char *const *argv) {
  const char *const *local_argv;
  int local_argc;
  int i;

  printf("argc=%d, argv=%p\n", argc, argv);
  for (i = 0; i < argc; ++i) {
    printf("argv[%d]=%s\n", i, argv[i]);
  }

  if (get_argv(&local_argv, &local_argc) != 0) {
    printf("FAIL: errno=(%d) %s\n", errno, strerror(errno));
    return 1;
  }

  printf("argc=%d, argv=%p\n", local_argc, local_argv);
  for (i = 0; i < local_argc; ++i) {
    printf("argv[%d]=%s\n", i, local_argv[i]);
  }

  return 0;
}
