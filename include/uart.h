#ifndef UART_H
#define UART_H

#define PORT 0x3f8          // COM1
#define SERIAL_BUF_SIZE 256


int init_serial();
char read_serial();
void write_serial(char a);

#endif