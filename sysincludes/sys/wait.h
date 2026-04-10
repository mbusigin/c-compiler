#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

typedef int pid_t;

#define WNOHANG 1
#define WUNTRACED 2

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

#define WIFEXITED(status) (((status) & 0x7f) == 0)
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFSIGNALED(status) (((status) & 0x7f) != 0)
#define WTERMSIG(status) ((status) & 0x7f)

#endif
