#include <unios/clock.h>
#include <unios/syscall.h>
#include <unios/proc.h>
#include <unios/interrupt.h>
#include <unios/kstate.h>
#include <arch/x86.h>
#include <sys/defs.h>

int system_ticks;

void clock_handler(int irq) {
    ++system_ticks;
    if (kstate_on_init) { return; }
    --p_proc_current->pcb.live_ticks;
    wakeup_exclusive(&system_ticks);
}

void init_sysclk() {
    //! use 8253 PIT timer0 as system clock
    outb(TIMER_MODE, RATE_GENERATOR);
    outb(TIMER0, (uint8_t)((TIMER_FREQ / SYSCLK_FREQ_HZ) >> 0));
    outb(TIMER0, (uint8_t)((TIMER_FREQ / SYSCLK_FREQ_HZ) >> 8));
    system_ticks = 0;

    //! enable clock irq for 8259A
    put_irq_handler(CLOCK_IRQ, clock_handler);
    enable_irq(CLOCK_IRQ);
}

int do_get_ticks() {
    return system_ticks;
}
