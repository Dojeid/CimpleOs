#include "kernel/signal.h"
#include "kernel/process.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"
#include <stddef.h>

static signal_table_t g_signal_tables[MAX_PROCESSES];
static signal_handler_t g_handlers[32];

void signal_init_table(signal_table_t* table) {
    if (!table) return;
    table->action = SIG_DFL;
    table->handler = NULL;
    table->pending_mask = 0;
    table->blocked_mask = 0;
}

int signal_send(uint32_t pid, int sig) {
    if (sig < 1 || sig > 31) return -1;
    if (pid >= MAX_PROCESSES) return -1;
    
    // We just use the pid as an index to the static array for simplicity,
    // assuming pid < MAX_PROCESSES and corresponds to slot index.
    g_signal_tables[pid].pending_mask |= (1 << sig);
    return 0;
}

int signal_raise(int sig) {
    process_t* current = process_get_current();
    if (!current) return -1;
    return signal_send(current->pid, sig);
}

void signal_deliver_pending(signal_table_t* table) {
    if (!table) return;
    process_t* current = process_get_current();
    if (!current) return;

    for (int i = 1; i <= 31; i++) {
        if ((table->pending_mask & (1 << i)) && !(table->blocked_mask & (1 << i))) {
            table->pending_mask &= ~(1 << i);
            
            if (g_handlers[i]) {
                g_handlers[i](i);
            } else {
                // Default handlers
                if (i == SIGKILL || i == SIGTERM || i == SIGSEGV || i == SIGINT || i == SIGQUIT) {
                    process_kill(current->pid);
                } else if (i == SIGSTOP) {
                    current->state = PROCESS_STATE_BLOCKED;
                } else if (i == SIGCONT) {
                    current->state = PROCESS_STATE_READY;
                }
            }
        }
    }
}

void signal_set_handler(int sig, signal_handler_t handler) {
    if (sig >= 1 && sig <= 31) {
        g_handlers[sig] = handler;
    }
}

void signal_block(int sig) {
    process_t* current = process_get_current();
    if (!current) return;
    if (sig >= 1 && sig <= 31) {
        g_signal_tables[current->pid].blocked_mask |= (1 << sig);
    }
}

void signal_unblock(int sig) {
    process_t* current = process_get_current();
    if (!current) return;
    if (sig >= 1 && sig <= 31) {
        g_signal_tables[current->pid].blocked_mask &= ~(1 << sig);
    }
}

int signal_is_pending(signal_table_t* table, int sig) {
    if (!table) return 0;
    if (sig >= 1 && sig <= 31) {
        return (table->pending_mask & (1 << sig)) != 0;
    }
    return 0;
}
