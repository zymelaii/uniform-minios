#pragma once

#include <stdbool.h>

int  get_pid();
int  get_ppid();
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

bool isxdigit(int c);
bool isupper(int c);
bool isspace(int c);
bool ispunct(int c);
bool isprint(int c);
bool islower(int c);
bool isgraph(int c);
bool isdigit(int c);
bool iscntrl(int c);
bool isblank(int c);
bool isascii(int c);
bool isalpha(int c);
bool isalnum(int c);

int toupper(int c);
int tolower(int c);
