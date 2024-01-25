#pragma once

#include <stdint.h>

typedef struct {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} ARDS_t;

typedef struct {
    uint8_t  video_card[4];
    uint8_t  vga_info[6];
    uint32_t ARDS_count;
    ARDS_t   ARDS_buffer[0];
} __attribute__((packed)) device_info_t;
