#include "kernel/process.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

extern void process_entry_trampoline(void);

static process_t processes[MAX_PROCESSES];
static uint32_t current_pid = 0; // Index into processes[], not the public PID.
static uint32_t next_pid = 1;

static uint64_t irq_save(void) {
    uint64_t flags;
    asm volatile("pushfq; pop %0; cli" : "=r"(flags) :: "memory");
    return flags;
}

static void irq_restore(uint64_t flags) {
    if (flags & 0x200) {
        asm volatile("sti" ::: "memory");
    }
}

static int process_slot_is_valid(int idx) {
    if (idx < 0 || idx >= MAX_PROCESSES) return 0;
    if (idx == 0) return processes[0].state != PROCESS_STATE_UNUSED;
    return processes[idx].pid != 0 && processes[idx].state != PROCESS_STATE_UNUSED;
}

static int process_find_slot_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_slot_is_valid(i) && processes[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

static int process_find_free_slot(void) {
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROCESS_STATE_UNUSED && processes[i].pid == 0) {
            return i;
        }
    }
    return -1;
}

static int process_has_child(process_t* parent, process_t* child, int32_t wait_pid) {
    if (!parent || !child) return 0;
    if (!process_slot_is_valid((int)(child - processes))) return 0;
    if (child->parent_pid != parent->pid) return 0;
    return wait_pid == PROCESS_WAIT_ANY || child->pid == (uint32_t)wait_pid;
}

static void process_close_fds(process_t* proc) {
    if (!proc) return;
    for (int i = 0; i < MAX_PROCESS_FDS; i++) {
        if (proc->fd_table[i]) {
            vfs_close(proc->fd_table[i]);
            proc->fd_table[i] = 0;
        }
    }
}

static void process_reap_slot(int idx) {
    if (idx <= 0 || idx >= MAX_PROCESSES) return;

    if (processes[idx].kernel_stack_base) {
        kfree((void*)processes[idx].kernel_stack_base);
    }

    memset(&processes[idx], 0, sizeof(process_t));
    processes[idx].state = PROCESS_STATE_UNUSED;
}

static void process_wake_waiters(uint32_t child_pid) {
    int child_slot = process_find_slot_by_pid(child_pid);
    if (child_slot < 0) return;

    process_t* child = &processes[child_slot];
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t* waiter = &processes[i];
        if (waiter->state != PROCESS_STATE_BLOCKED) continue;
        if (waiter->pid != child->parent_pid) continue;
        if (waiter->wait_pid != PROCESS_WAIT_ANY && waiter->wait_pid != (int)child_pid) continue;

        waiter->wait_result = (int)child_pid;
        waiter->wait_pid = PROCESS_WAIT_ANY;
        waiter->state = PROCESS_STATE_READY;
        if (waiter->saved_rsp) {
            ((cpu_registers_t*)waiter->saved_rsp)->rax = child_pid;
        }
    }
}

static int process_stack_contains(process_t* proc, uint64_t ptr) {
    if (!proc || !proc->kernel_stack_base || !proc->kernel_stack_size) return 0;
    return ptr >= proc->kernel_stack_base &&
           ptr < proc->kernel_stack_base + proc->kernel_stack_size;
}

static void process_rebase_stack_copy(uint64_t old_base, uint64_t new_base, uint64_t size) {
    uint64_t old_end = old_base + size;
    int64_t delta = (int64_t)(new_base - old_base);
    uint64_t* words = (uint64_t*)new_base;
    uint64_t count = size / sizeof(uint64_t);

    for (uint64_t i = 0; i < count; i++) {
        uint64_t value = words[i];
        if (value >= old_base && value < old_end) {
            words[i] = (uint64_t)((int64_t)value + delta);
        }
    }
}

void process_init(void) {
    memset(processes, 0, sizeof(processes));
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].state = PROCESS_STATE_UNUSED;
    }

    processes[0].pid = 0;
    strcpy(processes[0].name, "kernel_main");
    processes[0].state = PROCESS_STATE_RUNNING;
    processes[0].priority = 1;
    processes[0].wait_pid = PROCESS_WAIT_ANY;
    current_pid = 0;
    next_pid = 1;

    vga_print("[Scheduler] Preemptive scheduler initialized (PID 0: kernel_main).\n");
}

process_t* process_create(const char* name, void (*entry_point)(void)) {
    if (!name || !entry_point) return 0;

    uint64_t flags = irq_save();
    int slot = process_find_free_slot();
    if (slot < 0) {
        irq_restore(flags);
        return 0;
    }

    uint8_t* stack = (uint8_t*)kmalloc(PROCESS_KERNEL_STACK_SIZE);
    if (!stack) {
        irq_restore(flags);
        return 0;
    }
    memset(stack, 0, PROCESS_KERNEL_STACK_SIZE);

    process_t* proc = &processes[slot];
    memset(proc, 0, sizeof(process_t));

    proc->pid = next_pid++;
    strncpy(proc->name, name, PROCESS_NAME_LEN - 1);
    proc->name[PROCESS_NAME_LEN - 1] = '\0';
    proc->state = PROCESS_STATE_READY;
    proc->priority = 1;
    proc->kernel_stack_base = (uint64_t)stack;
    proc->kernel_stack_size = PROCESS_KERNEL_STACK_SIZE;
    proc->parent_pid = processes[current_pid].pid;
    proc->wait_pid = PROCESS_WAIT_ANY;
    proc->wait_result = 0;

    uint64_t stack_top = (uint64_t)(stack + PROCESS_KERNEL_STACK_SIZE);
    cpu_registers_t* regs = (cpu_registers_t*)(stack_top - sizeof(cpu_registers_t));
    memset(regs, 0, sizeof(cpu_registers_t));

    regs->gs = 0x10;
    regs->fs = 0x10;
    regs->es = 0x10;
    regs->ds = 0x10;
    regs->rdi = (uint64_t)entry_point;
    regs->int_no = 32;
    regs->err_code = 0;
    regs->rip = (uint64_t)process_entry_trampoline;
    regs->cs = 0x08;
    regs->rflags = 0x202;
    regs->rsp = stack_top;
    regs->ss = 0x10;

    proc->stack_top = (uint64_t)regs;
    proc->saved_rsp = (uint64_t)regs;

    irq_restore(flags);
    return proc;
}

uint64_t schedule(uint64_t current_rsp) {
    uint64_t flags = irq_save();

    if (current_pid >= MAX_PROCESSES || !process_slot_is_valid((int)current_pid)) {
        current_pid = 0;
    }

    process_t* current = &processes[current_pid];
    current->stack_top = current_rsp;
    current->saved_rsp = current_rsp;

    if (current->state == PROCESS_STATE_RUNNING) {
        current->state = PROCESS_STATE_READY;
        current->cpu_time_ms += 10;
    }

    uint32_t start = (current_pid + 1) % MAX_PROCESSES;
    for (int count = 0; count < MAX_PROCESSES; count++) {
        uint32_t idx = (start + count) % MAX_PROCESSES;
        process_t* candidate = &processes[idx];
        if (!process_slot_is_valid((int)idx)) continue;
        if (candidate->state != PROCESS_STATE_READY) continue;
        if (!candidate->saved_rsp && idx != 0) continue;

        candidate->state = PROCESS_STATE_RUNNING;
        current_pid = idx;
        uint64_t next_rsp = candidate->saved_rsp ? candidate->saved_rsp : current_rsp;
        irq_restore(flags);
        return next_rsp;
    }

    if (current->state == PROCESS_STATE_READY) {
        current->state = PROCESS_STATE_RUNNING;
        irq_restore(flags);
        return current_rsp;
    }

    if (process_slot_is_valid(0) && processes[0].saved_rsp) {
        processes[0].state = PROCESS_STATE_RUNNING;
        current_pid = 0;
        uint64_t idle_rsp = processes[0].saved_rsp;
        irq_restore(flags);
        return idle_rsp;
    }

    irq_restore(flags);
    return current_rsp;
}

void process_yield(void) {
    asm volatile("int $32");
}

void process_exit(int code) {
    uint64_t flags = irq_save();
    process_t* current = &processes[current_pid];
    current->exit_code = code;
    process_close_fds(current);
    current->state = PROCESS_STATE_TERMINATED;
    process_wake_waiters(current->pid);
    irq_restore(flags);

    process_yield();
    for (;;) {
        asm volatile("hlt");
    }
}

uint64_t process_exit_from_frame(int code, cpu_registers_t* frame) {
    uint64_t flags = irq_save();
    process_t* current = &processes[current_pid];

    current->exit_code = code;
    current->stack_top = (uint64_t)frame;
    current->saved_rsp = (uint64_t)frame;
    process_close_fds(current);
    current->state = PROCESS_STATE_TERMINATED;
    process_wake_waiters(current->pid);
    irq_restore(flags);

    return schedule((uint64_t)frame);
}

int64_t process_fork_from_frame(cpu_registers_t* frame) {
    if (!frame) return -1;

    uint64_t flags = irq_save();
    process_t* parent = &processes[current_pid];

    if (parent->pid == 0 || !process_stack_contains(parent, (uint64_t)frame)) {
        irq_restore(flags);
        return -1;
    }

    int slot = process_find_free_slot();
    if (slot < 0) {
        irq_restore(flags);
        return -1;
    }

    uint8_t* child_stack = (uint8_t*)kmalloc((size_t)parent->kernel_stack_size);
    if (!child_stack) {
        irq_restore(flags);
        return -1;
    }

    memcpy(child_stack, (void*)parent->kernel_stack_base, (size_t)parent->kernel_stack_size);

    uint64_t old_base = parent->kernel_stack_base;
    uint64_t new_base = (uint64_t)child_stack;
    uint64_t offset = (uint64_t)frame - old_base;
    uint64_t child_rsp = new_base + offset;

    process_rebase_stack_copy(old_base, new_base, parent->kernel_stack_size);

    process_t* child = &processes[slot];
    memset(child, 0, sizeof(process_t));
    child->pid = next_pid++;
    strncpy(child->name, parent->name, PROCESS_NAME_LEN - 1);
    child->name[PROCESS_NAME_LEN - 1] = '\0';
    if (strlen(child->name) < PROCESS_NAME_LEN - 6) {
        strcat(child->name, "_fork");
    }
    child->state = PROCESS_STATE_READY;
    child->priority = parent->priority;
    child->kernel_stack_base = new_base;
    child->kernel_stack_size = parent->kernel_stack_size;
    child->stack_top = child_rsp;
    child->saved_rsp = child_rsp;
    child->parent_pid = parent->pid;
    child->wait_pid = PROCESS_WAIT_ANY;

    for (int i = 0; i < MAX_PROCESS_FDS; i++) {
        child->fd_table[i] = parent->fd_table[i];
    }

    ((cpu_registers_t*)child_rsp)->rax = 0;
    int64_t child_pid = child->pid;
    irq_restore(flags);
    return child_pid;
}

int64_t process_waitpid_from_frame(int32_t pid, cpu_registers_t* frame, uint64_t* next_rsp) {
    if (next_rsp) *next_rsp = 0;
    if (!frame) return -1;

    uint64_t flags = irq_save();
    process_t* current = &processes[current_pid];

    int found_child = 0;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        process_t* child = &processes[i];
        if (!process_has_child(current, child, pid)) continue;

        found_child = 1;
        if (child->state == PROCESS_STATE_TERMINATED) {
            int child_pid = (int)child->pid;
            process_reap_slot(i);
            irq_restore(flags);
            return child_pid;
        }
    }

    if (!found_child) {
        irq_restore(flags);
        return -1;
    }

    current->wait_pid = pid;
    current->wait_result = 0;
    current->stack_top = (uint64_t)frame;
    current->saved_rsp = (uint64_t)frame;
    current->state = PROCESS_STATE_BLOCKED;
    irq_restore(flags);

    if (next_rsp) {
        *next_rsp = schedule((uint64_t)frame);
    }
    return 0;
}

process_t* process_create_elf(const char* name, const char* path) {
    extern vfs_node_t* vfs_lookup(const char* path);
    extern int elf_validate_header(const void* buf, size_t size);
    extern uint64_t elf_load_executable(const void* buf, size_t size, uint64_t* out_entry);
    
    vfs_node_t* node = vfs_lookup(path);
    if (!node || !node->data) {
        printf("[Process] ELF file not found in VFS: %s\n", path);
        return 0;
    }
    
    uint64_t entry_point = 0;
    if (!elf_load_executable(node->data, (size_t)node->size, &entry_point)) {
        printf("[Process] Failed to parse/load ELF binary: %s\n", path);
        return 0;
    }
    
    process_t* proc = process_create(name, (void(*)(void))entry_point);
    if (proc) {
        printf("[Process] Spawned Ring 3 User-Mode Process '%s' (PID %u) @ 0x%lX\n", name, proc->pid, entry_point);
    }
    return proc;
}
process_t* process_get_current(void) {
    if (current_pid >= MAX_PROCESSES || !process_slot_is_valid((int)current_pid)) {
        return &processes[0];
    }
    return &processes[current_pid];
}

void process_list(char* buffer, uint32_t max_len) {
    if (!buffer || max_len == 0) return;
    buffer[0] = 0;

    strcat(buffer, "PID   PPID  STATE       CPU(ms) NAME\n");
    strcat(buffer, "---   ----  ---------   ------- --------------------\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!process_slot_is_valid(i)) continue;

        char line[96];
        const char* st_str = "UNKNOWN";
        switch (processes[i].state) {
            case PROCESS_STATE_UNUSED:     st_str = "UNUSED"; break;
            case PROCESS_STATE_READY:      st_str = "READY"; break;
            case PROCESS_STATE_RUNNING:    st_str = "RUNNING"; break;
            case PROCESS_STATE_BLOCKED:    st_str = "BLOCKED"; break;
            case PROCESS_STATE_TERMINATED: st_str = "ZOMBIE"; break;
        }
        sprintf(line, "%-5u %-5u %-11s %-7u %s\n",
                processes[i].pid,
                processes[i].parent_pid,
                st_str,
                processes[i].cpu_time_ms,
                processes[i].name);
        if (strlen(buffer) + strlen(line) < max_len) {
            strcat(buffer, line);
        }
    }
}
