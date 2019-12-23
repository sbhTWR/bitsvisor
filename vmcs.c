#include "vmcs.h"

/**
 * Allocates VMCS region.
 * [bits 30:0] revision identifier
 * [bit 31] shadow revision indicator
 * [byte offset 4 (to next 4 bytes)] VMX-abort indicator: A logical processor writes non-zero value in these bits if VM abort occurs
 * [byte offset 8] VMCS data (implementation-specific format)
 */ 

int __alloc_vmcs_region(uint64_t* vmcs_region) {
    vmcs_region = kzalloc(CUST_PAGE_SIZE, GFP_KERNEL);
    if (vmcs_region == NULL) {
        printk(KERN_INFO "[BITSV] Error allocating VMCS region");
        return 0;
    }
    
    /* [bits 31:0] */
    *(uint32_t *)vmcs_region = vmcs_rev_id();

    return 1;
}

/**
 * VMPTRLD - Load pointer to VMCS
 * Marks current-VMCS pointer valid and sets to the physical
 * address in the instruction operand. Fails if:
 * - operand is not properly aligned
 * - if unsupported physical-address bits are set
 * - if equal to the VMXON pointer
 * - revision identifier [bits 30:0] of the aligned memory do not match with process revision identifier
 */ 

static inline int __vmptrld(uint64_t* vmcs_region) {
    vmcs_pa = __pa(vmcs_region);
    uint8_t ret;

    asm volatile ("vmptrld %[pa]; setna %[ret]"
        : [ret]"=rm"(ret)
        : [pa]"m"(vmcs_pa)
        : "cc", "memory");

    return ret;    
}

/**
 * vmread asm
 */ 