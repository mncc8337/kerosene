#include "mem.h"

void pde_add_attrib(uint32_t* e, uint32_t attrib) {
    *e |= attrib;
}
void pde_del_attrib(uint32_t* e, uint32_t attrib) {
    *e &= ~attrib;
}
void pde_set_frame(uint32_t* e, uint32_t addr) {
    *e = (*e & ~I86_PDE_FRAME) | addr;
}
bool pde_is_present(uint32_t e) {
    return e & I86_PDE_PRESENT;
}
bool pde_is_user(uint32_t e) {
    return e & I86_PDE_USER;
}
bool pde_is_4mb(uint32_t e) {
    return e & I86_PDE_4MB;
}
bool pde_is_writable(uint32_t e) {
    return e & I86_PDE_WRITABLE;
}
uint32_t pde_pfn(uint32_t e) {
    return e & I86_PDE_FRAME;
}
// commented till i need it
// void pde_enable_global(uint32_t e) {
// }
