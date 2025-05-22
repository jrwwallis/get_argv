# get_argv
Derive `argv` and `argc` anywhere from ELF stack contents

A notable annoyance of the Linux ELF c runtime is that `argc` and `argv` are parameters to `main()` and thus only visible with the scope of `main()`.  It is not unusual to require access to `argc` and `argv` elsewhere in the stack, and in particular from library code where there is no control over `main()`.

This is addressed here in as general a manner as possible with the following assumptions:
1. ELF stack layout
2. Linux stack growth direction (downwards)

The rest of the code should be largely conformant to Posix and Open specs.

Care is taken to find the bounds of the stack and not to exceed those bounds in either direction.

### Note also

argv[0] can be obtained with the GNU extensions `program_invocation_name`/`program_invocation_short_name` 

```
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <errno.h>
extern char *program_invocation_name;
extern char *program_invocation_short_name;
```

https://linux.die.net/man/3/program_invocation_short_name
