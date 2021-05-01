#include <errno.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>

void _exit(int status) {
  while (1) {
    __asm__("hlt");
  }
}

caddr_t program_break, program_break_end;

void *sbrk(ptrdiff_t incr) {
  if (program_break == 0 || program_break + incr >= program_break_end) {
    errno = ENOMEM;
    return (void *)-1;
  }

  caddr_t prev_break = program_break;
  program_break += incr;
  return (void *)prev_break;
}

pid_t getpid(void) { return 1; }

int kill(pid_t pid, int sig) {
  errno = EINVAL;
  return -1;
}
