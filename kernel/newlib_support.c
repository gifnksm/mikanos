#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

void _exit(int status) {
  while (1) {
    __asm__("hlt");
  }
}

void *sbrk(ptrdiff_t incr) { return NULL; }

pid_t getpid(void) { return 1; }

int kill(pid_t pid, int sig) {
  errno = EINVAL;
  return -1;
}
