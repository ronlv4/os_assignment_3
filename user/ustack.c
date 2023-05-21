#include "ustack.h"
#include "kernel/riscv.h"

// Define the stack data structure
typedef struct {
    void* base;         // Base address of the stack
    void* top;          // Current top of the stack
    void* prev_top;     // Previous top of the stack (for rollback)
} ustack_t;

// Global ustack instance
ustack_t ustack;

void* ustack_malloc(uint32_t len) {
    // Check if the ustack data structure exists
    if (ustack.base) {
        // Allocate memory for the stack using sbrk()
        ustack.base = sbrk(0);
        ustack.top = ustack.base;
        ustack.prev_top = NULL;
    }

    // Calculate the size required for the new allocation
    uint32_t aligned_len = (len + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);

    // Check if the requested size exceeds the maximum allowed size
    if (aligned_len > 512) {
        return -1;
    }

    // Check if there is enough space on the stack to accommodate the new allocation
    if (ustack.top + aligned_len > ustack.base + PGSIZE) {
        // Request another page from the kernel using sbrk()
        void* new_page = sbrk(PGSIZE);
        if (new_page == (void*)-1) {
            return -1;
        }
    }

    // Allocate the buffer on the stack
    void* buffer = ustack.top;
    ustack.prev_top = ustack.top;
    ustack.top += aligned_len;

    return buffer;
}

int ustack_free() {
    // Check if the stack is empty
    if (ustack.prev_top == NULL) {
        return -1;
    }

    // Calculate the length of the last allocated buffer
    uint32_t len = (uint32_t)(ustack.top - ustack.prev_top);

    // Roll back the stack
    ustack.top = ustack.prev_top;
    ustack.prev_top = NULL;

    // Check if rollback crosses a page boundary
    if ((uint32_t)ustack.top < (uint32_t)ustack.base) {
        // Release the pages allocated by sbrk()
        sbrk(-PGSIZE);
    }

    return len;
}
