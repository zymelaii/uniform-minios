#pragma once

#include <stdbool.h>

int  get_pid();
int  get_ticks();
int  exec(const char *path);
int  execve(const char *path, char *const *argv, char *const *envp);
int  fork();
int  wait(int *wstatus1);
void exit(int exit_code);
int  killerabbit(int pid);
void yield();
void sleep(int n);
void wakeup(void *channel);
int  pthread_create(void *args);

bool         putenv(char *const *envp);
char *const *getenv();
