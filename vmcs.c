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
 * TODO: 
 * 1. Using vmread and vmwrite, set different control fields in VMCS for:
 *   -> VM-execution
 *   -> VM-exit 
 *   -> VM-entry
 * 2. Initialize guest and the host state area in VCMS (laborious job!)
 *    -> define functions for accessing CR3 and CR4 registers.
 *    -> set various segment registers values.
 * 3. Define functions for freeing allocated VMCS and VMXON regions using
 *    kfree()
 */ 


