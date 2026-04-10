#ifndef _UNISTD_H
#define _UNISTD_H

typedef unsigned long size_t;
typedef long ssize_t;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int close(int fd);
int dup2(int oldfd, int newfd);
int pipe(int pipefd[2]);
int fork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execvp(const char *file, char *const argv[]);
pid_t getpid(void);
pid_t getppid(void);
int waitpid(pid_t pid, int *status, int options);
unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usec);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int access(const char *pathname, int mode);
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

int unlink(const char *pathname);
int rmdir(const char *pathname);

#endif
