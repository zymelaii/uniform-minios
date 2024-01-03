#pragma once

int compare_exchange_strong(int *value, int expected, int desired);
int compare_exchange_weak(int *value, int expected, int desired);
