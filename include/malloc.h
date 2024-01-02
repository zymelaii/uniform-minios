#pragma once

void *kmalloc(int size);
void *kmalloc_4k();
void *malloc(int size);
void *malloc_4k();
void  free(void *ptr);
