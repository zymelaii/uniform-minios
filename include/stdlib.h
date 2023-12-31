#pragma once

int  get_pid();
int  get_ticks();
int  exec(char *path);
int  fork();
void exit(int exit_code);
void yield();
void sleep(int n);
void wakeup(void *channel);
int  pthread(void *arg);