#include <mem.h>

#include <string.h>

#define PAGE_FRAME_BITS 0xfffff000

// our page table VIRTUAL address can be get by adding 0xffc00000 (4*1024*1024*1023)
// with page directory index multiply by page size
// we can do this because we have recursive paging (see kernel_entry.asm)
// we need to use virtual address because accessing a physical address will result in a page fault
#define PAGE_TABLE_ADDR(idx) (page_table_t*)(0xffc00000 + idx * MMNGR_PAGE_SIZE)

// from kernel_entry.asm
extern void* kernel_pd_virt;

static page_directory_t* current_page_directory = 0;
static page_directory_t* kernel_page_directory = (page_directory_t*)((unsigned)&kernel_pd_virt - KERNEL_START);

static void page_entry_set_frame(uint32_t* pe, physical_addr_t addr) {
    *pe = (*pe & ~PAGE_FRAME_BITS) | addr;
}
static void page_entry_add_attrib(uint32_t* pe, uint16_t attrib) {
    *pe |= (attrib & 0xfff);
}
static void page_entry_del_attrib(uint32_t* pe, uint16_t attrib) {
    *pe &= ~(attrib & 0xfff);
}

static void map_temporary_pd(page_directory_t* pd) {
    // manual mapping for efficiency
    page_table_t* pt = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX(VMMNGR_TEMP_PD));
    pte_t* pte = PAGE_TABLE_LOOKUP(pt, VMMNGR_TEMP_PD);
    *pte = (physical_addr_t)pd | PTE_PRESENT | PTE_WRITABLE;
    vmmngr_flush_tlb_entry(VMMNGR_TEMP_PD);
}

static void unmap_temporary_pd() {
    page_table_t* pt = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX(VMMNGR_TEMP_PD));
    pte_t* pte = PAGE_TABLE_LOOKUP(pt, VMMNGR_TEMP_PD);
    *pte = 0;
    vmmngr_flush_tlb_entry(VMMNGR_TEMP_PD);
}

static void map_temporary_pt(physical_addr_t phys) {
    pte_t* pte = PAGE_TABLE_LOOKUP(PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX(VMMNGR_TEMP_TABLE)), VMMNGR_TEMP_TABLE);
    *pte = phys | PTE_PRESENT | PTE_WRITABLE;
    vmmngr_flush_tlb_entry(VMMNGR_TEMP_TABLE);
}

static void unmap_temporary_pt() {
    page_table_t* pt = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX(VMMNGR_TEMP_TABLE));
    pte_t* pte = PAGE_TABLE_LOOKUP(pt, VMMNGR_TEMP_TABLE);
    *pte = 0;
    vmmngr_flush_tlb_entry(VMMNGR_TEMP_TABLE);
}

page_directory_t* vmmngr_get_page_directory() {
    return current_page_directory;
}

page_directory_t* vmmngr_get_kernel_page_directory() {
    return kernel_page_directory;
}

physical_addr_t vmmngr_to_physical_addr(page_directory_t* page_directory, virtual_addr_t virt) {
    bool mapped_temp_table = false;

    page_directory_t* virt_pd;
    if(page_directory == NULL) virt_pd = (page_directory_t*)VMMNGR_PD;
    else {
        map_temporary_pd(page_directory);
        virt_pd = (page_directory_t*)VMMNGR_TEMP_PD;
    }

    bool not_present = false;

    pde_t* pde = PAGE_DIRECTORY_LOOKUP(virt_pd, virt);
    if(!(*pde & PDE_PRESENT)) {
        not_present = true;
        goto clean;
    }

    // map the page table to access it if the pd is inactive
    page_table_t* table_to_modify;
    if(page_directory) {
        physical_addr_t phys = (*pde) & PAGE_FRAME_BITS;
        map_temporary_pt(phys);
        table_to_modify = (page_table_t*)VMMNGR_TEMP_TABLE;
        mapped_temp_table = true;
    } else {
        table_to_modify = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
    }


    pte_t* pte = PAGE_TABLE_LOOKUP(table_to_modify, virt);
    if(!(*pte & PTE_PRESENT)) {
        not_present = true;
        goto clean;
    }

clean:
    if(mapped_temp_table)
        unmap_temporary_pt();
    if(page_directory)
        unmap_temporary_pd();
    if(not_present)
        return 0;

    return (physical_addr_t)(*pte & PAGE_FRAME_BITS);
}

MEM_ERR vmmngr_map(page_directory_t* page_directory, physical_addr_t phys, virtual_addr_t virt, unsigned flags) {
    asm volatile("cli");
    page_directory_t* virt_pd;
    if(page_directory == NULL) virt_pd = (page_directory_t*)VMMNGR_PD;
    else {
        map_temporary_pd(page_directory);
        virt_pd = (page_directory_t*)VMMNGR_TEMP_PD;
    }

    pde_t* pde = PAGE_DIRECTORY_LOOKUP(virt_pd, virt);

    page_table_t* table_to_modify;

    bool mapped_temp_table = false;
    bool new_table = false;
    // if the page table is not present then allocate it
    if(!(*pde & PDE_PRESENT)) {
        physical_addr_t new_phys = (physical_addr_t)pmmngr_alloc_block();
        if(!new_phys) {
            if(page_directory)
                unmap_temporary_pd();
            asm volatile("sti");
            return ERR_MEM_OOM;
        }
        new_table = true;

        // create a new entry
        page_entry_add_attrib(pde, flags | PDE_PRESENT); // the first few PDE and PTE flags are the same so we can do this
        page_entry_set_frame(pde, new_phys);
        
        // reloading cr3 to flush the entire directory cache
        // to clear the non-presen state in the cache
        // else it will give a page fault when accessing this same pde on
        // the same iteration
        if(!page_directory) {
            asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" ::: "eax", "memory");
        }

        // we cant access the new table via its final recursive address.
        // we must map its physical page (new_phys) to another temporary
        // virtual address that is valid in the current page directory.
        
        // map our temporary page table to the current pd to edit
        map_temporary_pt(new_phys);
        mapped_temp_table = true;
        table_to_modify = (page_table_t*)VMMNGR_TEMP_TABLE;
    } else {
        if(page_directory) {
            // pd is inactive, so we must map the table temporarily
            physical_addr_t existing_phys = (*pde) & PAGE_FRAME_BITS;
            map_temporary_pt(existing_phys);
            mapped_temp_table = true;
            table_to_modify = (page_table_t*)VMMNGR_TEMP_TABLE;
        } else {
            table_to_modify = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
        }
    }

    // clear the table if we have just created it
    if(new_table)
        memset(table_to_modify, 0, sizeof(page_table_t));

    pte_t* pte = PAGE_TABLE_LOOKUP(table_to_modify, virt);
    page_entry_add_attrib(pte, PTE_PRESENT | flags);
    page_entry_set_frame(pte, phys);

    vmmngr_flush_tlb_entry(virt);

    // unmap the temporary page table
    if(mapped_temp_table) 
        unmap_temporary_pt();
    if(page_directory)
        unmap_temporary_pd();

    asm volatile("sti");
    return ERR_MEM_SUCCESS;
}

void vmmngr_unmap(page_directory_t* page_directory, virtual_addr_t virt) {
    bool mapped_temp_table = false;

    page_directory_t* virt_pd;
    if(page_directory == NULL) virt_pd = (page_directory_t*)VMMNGR_PD;
    else {
        map_temporary_pd(page_directory);
        virt_pd = (page_directory_t*)VMMNGR_TEMP_PD;
    }

    pde_t* pde = PAGE_DIRECTORY_LOOKUP(virt_pd, virt);
    if(!(*pde & PDE_PRESENT)) goto clean;

    // map the page table to access it if the pd is inactive
    page_table_t* table_to_modify;
    if(page_directory) {
        physical_addr_t phys = (*pde) & PAGE_FRAME_BITS;
        map_temporary_pt(phys);
        table_to_modify = (page_table_t*)VMMNGR_TEMP_TABLE;
        mapped_temp_table = true;
    } else {
        table_to_modify = PAGE_TABLE_ADDR(PAGE_DIRECTORY_INDEX((uint32_t)virt));
    }

    pte_t* pte = PAGE_TABLE_LOOKUP(table_to_modify, virt);
    if(!(*pte & PTE_PRESENT)) goto clean;

    // clear all attrib
    *pte = 0x0;

    vmmngr_flush_tlb_entry(virt);

clean:
    if(mapped_temp_table)
        unmap_temporary_pt();
    if(page_directory)
        unmap_temporary_pd();
}

MEM_ERR vmmngr_alloc_page(pte_t* pte) {
    void* p = pmmngr_alloc_block();
    if(!p) return ERR_MEM_OOM;

    page_entry_set_frame(pte, (physical_addr_t)p);
    page_entry_add_attrib(pte, PTE_PRESENT);

    return ERR_MEM_SUCCESS;
}

void vmmngr_free_page(pte_t* pte) {
    void* p = (void*)(*pte & PAGE_FRAME_BITS);
    if(p) pmmngr_free_block(p);

    page_entry_del_attrib(pte, PTE_PRESENT);
}

page_directory_t* vmmngr_alloc_page_directory() {
    page_directory_t* pd = (page_directory_t*)pmmngr_alloc_block();
    if(pd == NULL) return NULL;

    map_temporary_pd(pd);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_TEMP_PD;

    // copy the current page directory
    page_directory_t* kernel_pd = (page_directory_t*)VMMNGR_PD;
    memcpy(virt_pd, kernel_pd, sizeof(page_directory_t));

    // set final entry to itself for recursive paging
    pde_t* pde = &virt_pd->entry[1023];
    page_entry_add_attrib(pde, PDE_PRESENT | PDE_WRITABLE);
    page_entry_set_frame(pde, (physical_addr_t)pd);

    // map itself to VMMNGR_PD
    vmmngr_map(pd, (physical_addr_t)pd, VMMNGR_PD, PTE_PRESENT | PTE_WRITABLE);

    unmap_temporary_pd();

    return pd;
}

// free all memory lower than KERNEL_START of page_directory and itself
void vmmngr_free_page_directory(page_directory_t* page_directory) {
    map_temporary_pd(page_directory);
    page_directory_t* virt_pd = (page_directory_t*)VMMNGR_TEMP_PD;

    for(unsigned i = 0; i < 1024; i++) {
        // only free page table that map to address lower than KERNEL_START
        if(i * MMNGR_PAGE_SIZE * 1024 >= KERNEL_START) break;

        if(virt_pd->entry[i] == 0) continue;
        physical_addr_t phys_table = (virt_pd->entry[i] & PAGE_FRAME_BITS);
        vmmngr_map(NULL, phys_table, VMMNGR_TEMP_TABLE, PTE_PRESENT | PTE_WRITABLE);
        page_table_t* virt_table = (page_table_t*)VMMNGR_TEMP_TABLE;

        // free memory to map that page table
        for(int j = 0; j < 1024; j++) {
            physical_addr_t frame = virt_table->entry[j] & PAGE_FRAME_BITS;
            if(frame)
                pmmngr_free_block((void*)frame);
        }

        // finally free the table
        pmmngr_free_block((void*)phys_table);
        unmap_temporary_pt();
    }

    pmmngr_free_block(page_directory);

    unmap_temporary_pd();
}

void vmmngr_switch_page_directory(page_directory_t* dir) {
    if(current_page_directory == dir) return;

    current_page_directory = dir;
    asm volatile("mov %0, %%cr3" : : "r" (dir));
}

void vmmngr_flush_tlb_entry(virtual_addr_t addr) {
	asm volatile("invlpg (%0)" : : "b" ((void*)addr) : "memory");
}

void vmmngr_init() {
    current_page_directory = kernel_page_directory;
}
