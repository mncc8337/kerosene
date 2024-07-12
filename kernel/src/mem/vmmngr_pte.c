#include "mem.h"

void pte_add_attrib(uint32_t* e, uint32_t attrib) {
    *e |= attrib;
}
void pte_del_attrib(uint32_t* e, uint32_t attrib) {
    *e &= ~attrib;
}
void pte_set_frame(uint32_t* e, uint32_t addr) {
    *e = (*e & ~I86_PTE_FRAME) | addr;
}
bool pte_is_present(uint32_t e) {
    return e & I86_PTE_PRESENT;
}
bool pte_is_writable(uint32_t e) {
    return e & I86_PTE_WRITABLE;
}
uint32_t pte_pfn(uint32_t e) {
    return e & I86_PTE_FRAME;
}
