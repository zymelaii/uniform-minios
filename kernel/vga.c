#include <unios/vga.h>
#include <unios/assert.h>
#include <arch/x86.h>
#include <stdlib.h>
#include <math.h>

typedef vga_textmode_char_t  _char_t;
typedef vga_textmode_state_t _state_t;

#pragma GCC push_options
#pragma GCC optimize("O0")

void vga_set_crtc_data_unsafe(int index, uint8_t data) {
    outb(VGA_REG_CRTC_INDEX0, index);
    outb(VGA_REG_CRTC_DATA0, data);
}

uint8_t vga_get_crtc_data_unsafe(int index) {
    outb(VGA_REG_CRTC_INDEX0, index);
    return inb(VGA_REG_CRTC_DATA0);
}

uint8_t vga_get_crtc_data_direct_unsafe() {
    return inb(VGA_REG_CRTC_DATA0);
}

void vga_set_crtc_data_direct_unsafe(uint8_t data) {
    outb(VGA_REG_CRTC_DATA0, data);
}

uint8_t vga_get_textmode_char_height_unsafe() {
    uint8_t data = vga_get_crtc_data_unsafe(VGA_CRTC_MAX_SCANLINE);
    return (data & 0x1f) + 1;
}

uint8_t vga_get_textmode_width_unsafe() {
    return vga_get_crtc_data_unsafe(VGA_CRTC_HDISPLAY_END) + 1;
}

uint8_t vga_get_textmode_height_unsafe() {
    uint16_t scanlines   = vga_get_display_end_scanline_unsafe();
    uint8_t  char_height = vga_get_textmode_char_height_unsafe();
    return scanlines / char_height;
}

uint16_t vga_get_display_end_scanline_unsafe() {
    uint16_t lsb  = vga_get_crtc_data_unsafe(VGA_CRTC_VDISPLAY_END);
    uint16_t msb  = vga_get_crtc_data_unsafe(VGA_CRTC_OVERFLOW);
    uint16_t data = ((msb & 0x40) << 3) | ((msb & 0x02) << 7) | (lsb & 0xff);
    return data + 1;
}

void vga_set_vmem_base_unsafe(uint16_t addr) {
    vga_set_crtc_data_unsafe(VGA_CRTC_START_ADDR_HI, (addr >> 8) & 0xff);
    vga_set_crtc_data_unsafe(VGA_CRTC_START_ADDR_LO, (addr >> 0) & 0xff);
}

void vga_set_cursor_shape_unsafe(uint8_t start_scanline, uint8_t end_scanline) {
    uint8_t start = vga_get_crtc_data_unsafe(VGA_CRTC_CURSOR_START);
    vga_set_crtc_data_direct_unsafe((start & 0xe0) | (start_scanline & 0x1f));
    uint8_t end = vga_get_crtc_data_unsafe(VGA_CRTC_CURSOR_END);
    vga_set_crtc_data_direct_unsafe((end & 0xe0) | (end_scanline & 0x1f));
}

void vga_set_cursor_skew_unsafe(uint8_t skew) {
    uint8_t data = vga_get_crtc_data_unsafe(VGA_CRTC_CURSOR_END);
    vga_set_crtc_data_direct_unsafe((data & 0x9f) | ((skew & 0x03) << 5));
}

void vga_set_cursor_visible_unsafe(bool visible) {
    uint8_t disabled = (uint8_t)!visible;
    uint8_t data     = vga_get_crtc_data_unsafe(VGA_CRTC_CURSOR_START);
    vga_set_crtc_data_direct_unsafe((data & 0xdf) | (disabled << 5));
}

void vga_set_cursor_pos_unsafe(uint16_t pos) {
    vga_set_crtc_data_unsafe(VGA_CRTC_CURSOR_LOC_HI, (pos >> 8) & 0xff);
    vga_set_crtc_data_unsafe(VGA_CRTC_CURSOR_LOC_LO, (pos >> 0) & 0xff);
}

void vgatm_switch_to_unsafe(_state_t *state) {
    vgatm_sync_screen_unsafe(state);
    vgatm_sync_cursor_unsafe(state);
}

void vgatm_sync_screen_unsafe(_state_t *state) {
    int offset  = state->origin - (_char_t *)state->vmem_base;
    int phyaddr = state->vmem_base_phy + offset * sizeof(_char_t);
    vga_set_vmem_base_unsafe((phyaddr - VGA_VMEM3_BASE) / sizeof(_char_t));
}

void vgatm_update_cursor_pos_unsafe(_state_t *state) {
    vga_set_cursor_pos_unsafe(state->cursor_pos);
}

void vgatm_sync_cursor_unsafe(_state_t *state) {
    if (!state->cursor_enabled) {
        vga_set_cursor_visible_unsafe(false);
        return;
    }
    vga_set_cursor_shape_unsafe(state->cursor_begin, state->cursor_end);
    vga_set_cursor_visible_unsafe(true);
    vgatm_update_cursor_pos_unsafe(state);
}

#pragma GCC pop_options

void vgatm_flush_vmem(_state_t *state, uint16_t raw) {
    u16 *vmem = state->vmem_base;
    for (int i = 0; i < state->capacity; ++i) { vmem[i] = raw; }
}

void vgatm_fill_screen(_state_t *state, uint16_t raw) {
    int offset = state->origin - (_char_t *)state->vmem_base;
    int limit  = min(state->capacity - offset, state->width * state->height);
    for (int i = 0; i < limit; ++i) { state->origin[i].raw = raw; }
}

void vgatm_fill_line(_state_t *state, uint16_t lineno, uint16_t raw) {
    _char_t *origin = state->origin + lineno * state->width;
    int      offset = origin - (_char_t *)state->vmem_base;
    int      limit  = min(state->capacity - offset, state->width);
    for (int i = 0; i < limit; ++i) { origin[i].raw = raw; }
}

void vgatm_fill_absolute_line(_state_t *state, uint32_t lineno, uint16_t raw) {
    _char_t *origin = (_char_t *)state->vmem_base + lineno * state->width;
    int      offset = origin - (_char_t *)state->vmem_base;
    int      limit  = min(state->capacity - offset, state->width);
    for (int i = 0; i < limit; ++i) { origin[i].raw = raw; }
}

void vgatm_fill_current_line(_state_t *state, uint16_t raw) {
    uint16_t lineno = state->cursor_pos / state->width;
    vgatm_fill_line(state, lineno, raw);
}

void vgatm_fill_current_wrapped_line(_state_t *state, uint16_t raw) {
    int      lineno = state->cursor_pos / state->width;
    int      offset = state->origin - (_char_t *)state->vmem_base;
    int      from   = max(0, offset + state->real_line * state->width);
    int      to = min(state->capacity, offset + (lineno + 1) * state->width);
    _char_t *origin = state->vmem_base;
    for (int i = from; i < to; ++i) { origin[i].raw = raw; }
}

void vgatm_write_printable_char_direct(_state_t *state, char ch) {
    _char_t charcode = {
        .code = ch,
        .attr = state->origin[state->next_char_pos].attr,
    };
    if (state->use_last_attr) { charcode.attr = state->last_attr; }
    state->origin[state->next_char_pos].raw = charcode.raw;
}

void vgatm_write_printable_char_at_direct_unsafe(
    _state_t *state, int32_t pos, char ch) {
    _char_t charcode = {
        .code = ch,
        .attr = state->origin[pos].attr,
    };
    if (state->use_last_attr) { charcode.attr = state->last_attr; }
    state->origin[pos].raw = charcode.raw;
}

void vgatm_put_printable_char_direct(_state_t *state, char ch) {
    _char_t charcode = {
        .code = ch,
        .attr = state->origin[state->next_char_pos].attr,
    };
    if (state->use_last_attr) { charcode.attr = state->last_attr; }
    vgatm_put_raw(state, charcode.raw);
}

void vgatm_put_printable_str_direct(_state_t *state, const char *str) {
    const int     offset = state->origin - (_char_t *)state->vmem_base;
    const int     maxlen = state->capacity - offset - state->next_char_pos;
    const bool    use_last_attr = state->use_last_attr;
    const uint8_t last_attr     = state->last_attr;

    const char *p     = str;
    int         total = 0;

    _char_t *charcode = &state->origin[state->next_char_pos];

    while (total < maxlen && *p != '\0') {
        charcode->code = *p++;
        charcode->attr = use_last_attr ? last_attr : charcode->attr;
        ++total;
        ++charcode;
    }

    state->next_char_pos += total;
    state->last_char_pos  = state->next_char_pos - 1;
}

void vgatm_write_raw(_state_t *state, uint16_t raw) {
    state->origin[state->next_char_pos].raw = raw;
}

void vgatm_write_raw_at_unsafe(_state_t *state, int32_t pos, uint16_t raw) {
    state->origin[pos].raw = raw;
}

void vgatm_put_raw(_state_t *state, uint16_t raw) {
    state->origin[state->next_char_pos].raw = raw;
    state->last_char_pos                    = state->next_char_pos;
    int offset = state->origin - (_char_t *)state->vmem_base;
    state->next_char_pos =
        min(state->next_char_pos + 1, (int)(state->capacity - offset));
}

void vgatm_accept_ctrl_lf(_state_t *state) {
    int pos    = state->last_char_pos + state->width;
    int offset = state->origin - (_char_t *)state->vmem_base;
    int limit  = state->capacity - offset;
    //! NOTE: if out of range, simply truncate to the current line
    if (pos >= limit) { pos -= state->width; }
    state->real_line     = idiv_floor(pos, state->width);
    state->last_char_pos = pos;
    state->next_char_pos = pos;
}

void vgatm_accept_ctrl_cr(_state_t *state) {
    state->real_line     = idiv_floor(state->last_char_pos, state->width);
    state->last_char_pos = state->real_line * state->width;
    state->next_char_pos = state->last_char_pos;
}

void vgatm_accept_ctrl_bs(_state_t *state) {
    int offset           = state->origin - (_char_t *)state->vmem_base;
    int limit            = -offset;
    state->next_char_pos = max(state->next_char_pos - 1, limit);
    state->last_char_pos = state->next_char_pos;
    //! FIXME: expect floor division
    state->real_line = idiv_floor(state->last_char_pos, state->width);
    state->last_attr = state->origin[state->last_char_pos].attr;
}

void vgatm_accept_ctrl_ff(_state_t *state) {
    int offset        = state->origin - (_char_t *)state->vmem_base;
    int line          = idiv_floor(state->last_char_pos, state->width) + 1;
    int origin_offset = line * state->width;
    //! NOTE: if out of range, simply skip
    if (offset + origin_offset >= state->capacity) { return; }
    state->origin        += origin_offset;
    state->real_line      = 0;
    state->last_char_pos  = 0;
    state->next_char_pos  = 0;
}

bool vgatm_accept_char(_state_t *state, char ch) {
    if (isprint(ch)) {
        vgatm_put_printable_char_direct(state, ch);
        return true;
    }
    if (!iscntrl(ch)) { return false; }
    switch (ch) {
        case '\0': {
        } break;
        case '\n': {
            //! NOTE: simply inc lineno, hook this if requires entering newline
            vgatm_accept_ctrl_lf(state);
        } break;
        case '\r': {
            //! NOTE: simply cr, hook this if requires advanced cr
            vgatm_accept_ctrl_cr(state);
        } break;
        case '\b': {
            //! NOTE: simply backspace, hook this if requires bs limit
            vgatm_accept_ctrl_bs(state);
        } break;
        case '\f': {
            vgatm_accept_ctrl_ff(state);
        } break;
        case '\t': {
            //! NOTE: no enough clues to decide how to print a tab
            return false;
        } break;
        case '\e': {
            //! NOTE: use `vgatm_accept_csi_seq` instead
            return false;
        } break;
        default: {
            return false;
        } break;
    }
    return true;
}

int vgatm_accept_str_anyhow(_state_t *state, const char *str) {
    int         total_accepted = 0;
    const char *p              = str;
    while (*p != '\0') {
        total_accepted += vgatm_accept_char(state, *p++) ? 1 : 0;
    }
    return total_accepted;
}

int vgatm_accept_str_until_fail(_state_t *state, const char *str) {
    int         total = 0;
    const char *p     = str;
    while (*p != '\0') {
        if (!vgatm_accept_char(state, *p++)) { return total; }
        ++total;
    }
    return total;
}

int vgatm_accept_csi_seq(_state_t *state, const char *seq) {
    todo("vga text mode: resolve CSI sequence");
    return -1;
}
