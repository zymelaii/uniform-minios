#include <unios/serial.h>
#include <unios/assert.h>
#include <arch/x86.h>

int init_serial() {
    outb(PORT_COM1 + 1, 0x00); // Disable all interrupts
    outb(PORT_COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(PORT_COM1 + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(PORT_COM1 + 1, 0x00); //                  (hi byte)
    outb(PORT_COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(
        PORT_COM1 + 2, 0xc7); // Enable FIFO, clear them, with 14-byte threshold
    outb(PORT_COM1 + 4, 0x0b); // IRQs enabled, RTS/DSR set
    outb(PORT_COM1 + 4, 0x1e); // Set in loopback mode, test the serial chip
    outb(PORT_COM1 + 0, 0xae); // Test serial chip (send byte 0xAE and check if
                               // serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (inb(PORT_COM1 + 0) != 0xae) {
        panic("faulty serial");
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(PORT_COM1 + 4, 0x0f);
    return 0;
}

static inline int is_transmit_empty() {
    return inb(PORT_COM1 + 5) & 0x20;
}

static inline int serial_received() {
    return inb(PORT_COM1 + 5) & 0x01;
}

char serial_read() {
    while (!serial_received()) {}
    return inb(PORT_COM1);
}

void serial_write(char a) {
    while (!is_transmit_empty()) {}
    outb(PORT_COM1, a);
}
