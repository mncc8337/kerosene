#include "mem.h"

void pde_add_attrib(pde_t* e, uint32_t attrib) {
    *e |= attrib;
}
void pde_del_attrib(pde_t* e, uint32_t attrib) {
    *e &= ~attrib;
}
void pde_set_frame(pde_t* e, physical_addr_t addr) {
    *e = (*e & ~PDE_FRAME) | addr;
}
bool pde_is_present(pde_t e) {
    return e & PDE_PRESENT;
}
bool pde_is_user(pde_t e) {
    return e & PDE_USER;
}
bool pde_is_4mb(pde_t e) {
    return e & PDE_4MB;
}
bool pde_is_writable(pde_t e) {
    return e & PDE_WRITABLE;
}
uint32_t pde_pfn(pde_t e) {
    return e & PDE_FRAME;
}
// commented till i need it
// void pde_enable_global(pde_t e) {
// }
