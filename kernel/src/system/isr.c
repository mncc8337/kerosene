#include "system.h"
#include "pic.h"
#include "stdio.h"
#include "string.h"
#include "video.h"

static void* routines[IDT_MAX_DESCRIPTORS];
// from isr.asm
extern void* isr_table[IDT_MAX_DESCRIPTORS];

static char* exception_message[] = {
    "Division Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

static void page_fault_handler(regs_t* r) {
    // the faulting address is stored in the CR2 register
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    bool p    = r->err_code & 0x1;
    bool rw   = r->err_code & 0x2;
    bool us   = r->err_code & 0x4;
    bool rsvd = r->err_code & 0x8;
    bool id   = r->err_code & 0x10;
    bool pk   = r->err_code & 0x20;
    bool ss   = r->err_code & 0x40;
    bool sgx  = r->err_code & 0x8000;

    printf("page fault at address 0x%x\n", faulting_address);
    printf("flags: ");

    if(p) printf("present, ");
    else printf("not-present, ");

    if(rw) printf("read/write, ");
    else printf("read-only, ");

    if(us) printf("user, ");
    else printf("supervisor, ");

    if(rsvd) printf("reserved, ");

    if(id) printf("instruction fetch, ");
    else printf("data access, ");

    if(pk) printf("protection-key violation, ");
    if(ss) printf("shadow-stack access fault, ");
    if(sgx) printf("SGX violation, ");

    putchar('\n');
}

static void* exception_handlers[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    page_fault_handler,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// default exception handler
static void exception_handler(regs_t* r) {
    video_vesa_set_cursor(0);
    video_set_attr(video_rgb(VIDEO_WHITE), video_rgb(VIDEO_BLACK));
    printf("Exception: ");
    video_set_attr(video_rgb(VIDEO_LIGHT_RED), video_rgb(VIDEO_BLACK));
    puts(exception_message[r->int_no]);
    video_set_attr(video_rgb(VIDEO_WHITE), video_rgb(VIDEO_BLACK));
    printf("Error code: 0b%b\n", r->err_code);

    void (*handler)(regs_t*) = exception_handlers[r->int_no];
    if(handler) handler(r);

    stackframe_t stk = {(stackframe_t*)r->ebp, r->eip};
    kernel_panic(&stk);
}

// default ISR. every interrupt will be "handled" by this function
void isr_handler(regs_t* reg) {
    void (*handler)(regs_t*) = routines[reg->int_no];
    if(handler) handler(reg);

    // if it is an IRQ
    if(reg->int_no >= 32 && reg->int_no <= 47)
        pic_send_eoi(reg->int_no - 32);
}

void irq_install_handler(int irq, void (*handler)(regs_t*)) {
    if(irq > 15) return;
    routines[irq+32] = handler;
}
void irq_uninstall_handler(int irq) {
    if(irq > 15) return;
    routines[irq+32] = 0;
}

void isr_new_interrupt(int isr, void (*handler)(regs_t*), uint8_t flags) {
    idt_set_descriptor(isr, isr_table[isr], flags);
    routines[isr] = handler;
}

void isr_init() {
    // set exception handler
    for(unsigned char vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_table[vector], 0x8f);
        routines[vector] = exception_handler;
    }

    // set IRQ handler
    // send commands to the PIC to make IRQ0 to 15 be remapped to IDT entry 32 to 47
    pic_remap(32, 40); // master holds 8 entries so slave is mapped to 40
    for(int vector = 32; vector < 48; vector++)
        idt_set_descriptor(vector, isr_table[vector], 0x8e);

    memset(routines + 32, 0, 255 - 32);
}

