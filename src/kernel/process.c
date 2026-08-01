#include "kernel/process.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

static process_t processes[MAX_PROCESSES];
static uint32_t current_pid = 0;
static uint32_t next_pid = 1;

void process_init(void) {
    memset(processes, 0, sizeof(processes));
    
    // Process 0: System Idle Kernel Process
    processes[0].pid = 0;
    strcpy(processes[0].name, "kernel_idle");
    processes[0].state = PROCESS_STATE_RUNNING;
    processes[0].priority = 1;
    
    vga_print("[Scheduler] Process Scheduler initialized (PID 0: kernel_idle).\n");
}

process_t* process_create(const char* name, void (*entry_point)(void)) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_STATE_TERMINATED || processes[i].pid == 0) {
            processes[i].pid = next_pid++;
            strncpy(processes[i].name, name, PROCESS_NAME_LEN - 1);
            processes[i].state = PROCESS_STATE_READY;
            processes[i].priority = 1;
            processes[i].cpu_time_ms = 0;
            for (int j = 0; j < MAX_PROCESS_FDS; j++) {
                processes[i].fd_table[j] = 0;
            }
            
            // Allocate a 4KB stack
            uint8_t* stack = (uint8_t*)kmalloc(4096);
            memset(stack, 0, 4096);
            processes[i].stack_top = (uint64_t)(stack + 4096);
            
            // Set up initial interrupt frame
            cpu_registers_t* regs = (cpu_registers_t*)(processes[i].stack_top - sizeof(cpu_registers_t));
            regs->rip = (uint64_t)entry_point;
            regs->cs = 0x08; // Kernel code segment
            regs->rflags = 0x202; // Interrupts enabled
            regs->rsp = processes[i].stack_top;
            regs->ss = 0x10; // Kernel data segment
            regs->ds = 0x10;
            regs->es = 0x10;
            regs->fs = 0x10;
            regs->gs = 0x10;
            
            processes[i].stack_top = (uint64_t)regs;
            
            return &processes[i];
        }
    }
    return 0;
}

uint64_t schedule(uint64_t current_rsp) {
    if (processes[current_pid].state == PROCESS_STATE_RUNNING) {
        processes[current_pid].stack_top = current_rsp;
        processes[current_pid].state = PROCESS_STATE_READY;
    } else if (processes[current_pid].state == PROCESS_STATE_TERMINATED || 
               processes[current_pid].state == PROCESS_STATE_BLOCKED) {
        processes[current_pid].stack_top = current_rsp;
    }
    
    uint32_t next_p = (current_pid + 1) % MAX_PROCESSES;
    for (int count = 0; count < MAX_PROCESSES; count++) {
        if (processes[next_p].pid != 0 && processes[next_p].state == PROCESS_STATE_READY) {
            processes[next_p].state = PROCESS_STATE_RUNNING;
            current_pid = next_p;
            return processes[next_p].stack_top;
        }
        next_p = (next_p + 1) % MAX_PROCESSES;
    }
    
    // Fallback to idle if none ready
    if (processes[current_pid].state != PROCESS_STATE_RUNNING) {
        current_pid = 0;
        processes[0].state = PROCESS_STATE_RUNNING;
    }
    return processes[current_pid].stack_top;
}

void process_yield(void) {
    // Legacy yielding is now replaced by software interrupts or just waiting
    // For now we simulate an int $0x20 (timer tick) to yield
    asm volatile("int $32");
}

void process_exit(int code) {
    processes[current_pid].state = PROCESS_STATE_TERMINATED;
    process_yield();
}

process_t* process_get_current(void) {
    return &processes[current_pid];
}

void process_list(char* buffer, uint32_t max_len) {
    if (!buffer || max_len == 0) return;
    buffer[0] = 0;

    strcat(buffer, "PID   STATE       NAME\n");
    strcat(buffer, "---   ---------   --------------------\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].pid != 0 || i == 0) {
            char line[64];
            const char* st_str = "UNKNOWN";
            switch (processes[i].state) {
                case PROCESS_STATE_READY:      st_str = "READY"; break;
                case PROCESS_STATE_RUNNING:    st_str = "RUNNING"; break;
                case PROCESS_STATE_BLOCKED:    st_str = "BLOCKED"; break;
                case PROCESS_STATE_TERMINATED: st_str = "STOPPED"; break;
            }
            sprintf(line, "%-5u %-11s %s\n", processes[i].pid, st_str, processes[i].name);
            if (strlen(buffer) + strlen(line) < max_len) {
                strcat(buffer, line);
            }
        }
    }
}
