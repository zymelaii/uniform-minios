#include <unios/console.h>
#include <unios/layout.h>
#include <unios/tracing.h>
#include <unios/memory.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

void setup_vga_console(console_t* con) {
    assert(con != NULL);
    con->state.width          = vga_get_textmode_width_unsafe();
    con->state.height         = vga_get_textmode_height_unsafe();
    con->state.capacity       = VGA_VMEM3_SIZE / sizeof(vga_textmode_char_t);
    con->state.vmem_base_phy  = VGA_VMEM3_BASE;
    con->state.vmem_base      = K_PHY2LIN(con->state.vmem_base_phy);
    con->state.origin         = con->state.vmem_base;
    con->state.last_char_pos  = 0;
    con->state.next_char_pos  = 0;
    con->state.cursor_pos     = 0;
    con->state.real_line      = 0;
    con->state.last_attr      = VGATM_ATTR(white, black, false);
    con->state.cursor_begin   = vga_get_textmode_char_height_unsafe() - 2;
    con->state.cursor_end     = con->state.cursor_begin + 1;
    con->state.cursor_enabled = false;
    con->state.use_last_attr  = false;
    con->vmem_real            = con->state.vmem_base;
    con->vmem_buf             = NULL;
    con->wrlock               = 0;
}

bool vcon_is_foreground(console_t* con) {
    assert(con != NULL);
    return con->state.vmem_base == con->vmem_real;
}

bool vcon_is_offscreen(console_t* con) {
    assert(con != NULL);
    return con->state.vmem_base == con->vmem_buf;
}

void vcon_make_foreground(console_t* con) {
    typedef vga_textmode_char_t char_t;

    acquire(&con->wrlock);
    assert(con != NULL);
    if (vcon_is_offscreen(con)) {
        ptrdiff_t vmem_diff = con->state.origin - (char_t*)con->state.vmem_base;
        memcpy(
            con->vmem_real,
            con->vmem_buf,
            con->state.capacity * sizeof(char_t));
        con->state.origin    = (char_t*)con->vmem_real + vmem_diff;
        con->state.vmem_base = con->vmem_real;
    }
    vgatm_switch_to_unsafe(&con->state);
    assert(vcon_is_foreground(con));
    release(&con->wrlock);
}

void vcon_make_offscreen(console_t* con) {
    typedef vga_textmode_char_t char_t;

    acquire(&con->wrlock);
    assert(con != NULL);
    if (vcon_is_offscreen(con)) { return; }
    if (con->vmem_buf == NULL) {
        //! TODO: move to user space
        con->vmem_buf = kmalloc(con->state.capacity * sizeof(char_t));
        assert(con->vmem_buf != NULL);
    }
    ptrdiff_t vmem_diff = con->state.origin - (char_t*)con->state.vmem_base;
    memcpy(con->vmem_buf, con->vmem_real, con->state.capacity * sizeof(char_t));
    con->state.origin    = (char_t*)con->vmem_buf + vmem_diff;
    con->state.vmem_base = con->vmem_buf;
    assert(vcon_is_offscreen(con));
    release(&con->wrlock);
}

void vcon_scroll(console_t* con, int row_diff) {
    typedef vga_textmode_char_t  char_t;
    typedef vga_textmode_state_t state_t;

    state_t* state  = &con->state;
    char_t*  base   = state->vmem_base;
    int      origin = state->origin - base + row_diff * state->width;
    int      limit  = origin + state->width * state->height;

    int row_corrected = 0;
    if (origin < 0) {
        row_corrected = (-origin + state->width - 1) / state->width;

    } else if (limit > state->capacity) {
        row_corrected = -((limit - 1 - state->capacity) / state->width + 1);
    }
    row_diff += row_corrected;

    int diff              = row_diff * state->width;
    state->origin        += diff;
    state->cursor_pos    -= diff;
    state->last_char_pos -= diff;
    state->next_char_pos -= diff;
    state->real_line     -= row_diff;

    if (vcon_is_foreground(con)) {
        vgatm_sync_screen_unsafe(state);
        vgatm_sync_cursor_unsafe(state);
    }
}

static void _vcon_write(vga_textmode_state_t* state, char ch) {
    switch (ch) {
        case '\n': {
            vgatm_accept_ctrl_cr(state);
            vgatm_accept_ctrl_lf(state);
        } break;
        case '\b': {
            vgatm_accept_ctrl_bs(state);
            vgatm_write_printable_char_direct(state, ' ');
        } break;
        default: {
            vgatm_accept_char(state, ch);
        } break;
    }
}

void vcon_write(console_t* con, const char* buf, int size) {
    typedef vga_textmode_char_t  char_t;
    typedef vga_textmode_state_t state_t;

    acquire(&con->wrlock);

    state_t* state = &con->state;
    if (size == -1) { size = strlen(buf); }

    char_t* base   = state->vmem_base;
    int     offset = state->origin - base;
    assert(offset % state->width == 0);

    size_t copy_size = (state->capacity - state->width) * sizeof(char_t);
    int    limit     = state->capacity - offset;

    for (int i = 0; i < size; ++i) {
        char ch = buf[i];
        do {
            if (ch == '\b' || ch == '\r') { break; }
            int inc = ch == '\n' ? state->width : 1;
            if (state->last_char_pos + inc >= limit) {
                //! FIXME: sometimes perform bad if capacity too large
                memcpy(base, base + state->width, copy_size);
                int lineno = idiv_floor(state->last_char_pos, state->width);
                state->cursor_pos    -= state->width;
                state->last_char_pos -= state->width;
                state->next_char_pos -= state->width;
                --state->real_line;
                char_t* last_line = state->origin + lineno * state->width;
                for (int i = 0; i < state->width; ++i) {
                    last_line[i].code = ' ';
                }
            }
        } while (0);
        _vcon_write(state, ch);
    }

    int upper_bound = state->width * state->height;
    int row_diff    = 0;
    if (state->last_char_pos >= upper_bound) {
        row_diff = (state->last_char_pos - upper_bound + state->width - 1)
                 / state->width;
    } else if (state->last_char_pos < 0) {
        row_diff = -((-state->last_char_pos + state->width - 1) / state->width);
    }
    if (row_diff != 0) { vcon_scroll(con, row_diff); }

    release(&con->wrlock);
}

void vcon_clear_screen(console_t* con) {
    vga_textmode_char_t charcode = {
        .attr = con->state.last_attr,
        .code = ' ',
    };

    acquire(&con->wrlock);
    vgatm_fill_screen(&con->state, charcode.raw);
    release(&con->wrlock);
}
