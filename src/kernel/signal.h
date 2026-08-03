#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdint.h>

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGTERM   15
#define SIGCHLD   17
#define SIGSTOP   18
#define SIGCONT   19

typedef enum signal_action {
    SIG_DFL,
    SIG_IGN,
    SIG_HANDLER
} signal_action_t;

typedef void (*signal_handler_t)(int sig);

typedef struct {
    signal_action_t action;
    signal_handler_t handler;
    uint32_t pending_mask;
    uint32_t blocked_mask;
} signal_table_t;

void signal_init_table(signal_table_t* table);
int signal_send(uint32_t pid, int sig);
int signal_raise(int sig);
void signal_deliver_pending(signal_table_t* table);
void signal_set_handler(int sig, signal_handler_t handler);
void signal_block(int sig);
void signal_unblock(int sig);
int signal_is_pending(signal_table_t* table, int sig);

#endif // SIGNAL_H
