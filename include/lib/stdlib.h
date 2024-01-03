#pragma once

int  get_pid();
int  get_ticks();
int  exec(char *path);
int  fork();
int  wait(int *wstatus1);
void exit(int exit_code);
void yield();
void sleep(int n);
void wakeup(void *channel);
int  pthread(void *args);
