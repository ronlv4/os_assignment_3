#include "ustack.h"
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/riscv.h"

// Define the stack data structure
typedef struct {
    void* base;         // Base address of the stack
    void* top;          // Current top of the stack
    void* prev_top;     // Previous top of the stack (for rollback)
} ustack_t;

struct header
{
    uint len;
    uint dealloc_page;
    struct header* prev;
};


// Global ustack instance
static ustack_t ustack;
static struct header* top;

void* ustack_malloc(uint len) {
    if (top)
    {
        struct header* new_header = (struct header*)ustack.top;
        new_header->len = len;
        new_header->dealloc_page = 0;
        new_header->prev = top;
        top = new_header;
    }

    // Check if the ustack data structure exists
    if (ustack.base) {
        // Allocate memory for the stack using sbrk()
        ustack.base = sbrk(0);
        ustack.top = ustack.base;
        ustack.prev_top = 0;
    }

    // Calculate the size required for the new allocation
    uint aligned_len = (len + sizeof(uint) - 1) & ~(sizeof(uint) - 1);

    // Check if the requested size exceeds the maximum allowed size
    if (aligned_len > MAX_ALLOC) {
        return (void*)-1;
    }

    // Check if there is enough space on the stack to accommodate the new allocation
    if (ustack.top + aligned_len > ustack.base + PGSIZE) {
        // Request another page from the kernel using sbrk()
        void* new_page = sbrk(PGSIZE);
        if (new_page == (void*)-1) {
            return (void*)-1;
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
    if (ustack.prev_top == 0) {
        return -1;
    }

    // Calculate the length of the last allocated buffer
    uint len = (uint)(ustack.top - ustack.prev_top);

    // Roll back the stack
    ustack.top = ustack.prev_top;
    ustack.prev_top = 0;

    // Check if rollback crosses a page boundary
    if (ustack.top < ustack.base) {
        // Release the pages allocated by sbrk()
        sbrk(-PGSIZE);
    }

    return len;
}
