#include "mem.h"

void pte_add_attrib(pte_t* e, uint32_t attrib) {
    *e |= attrib;
}
void pte_del_attrib(pte_t* e, uint32_t attrib) {
    *e &= ~attrib;
}
void pte_set_frame(pte_t* e, physical_addr_t addr) {
    *e = (*e & ~PTE_FRAME) | addr;
}
bool pte_is_present(pte_t e) {
    return e & PTE_PRESENT;
}
bool pte_is_writable(pte_t e) {
    return e & PTE_WRITABLE;
}
uint32_t pte_pfn(pte_t e) {
    return e & PTE_FRAME;
}
