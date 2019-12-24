/**
 * bitsvisor.c
 * 
 * Kernel module that enables Linux to act as a VMM (hypervsior)
 * Leverges x86's VT-x technology to support virtualization.
 * 
 * @author   Shubham Tiwari <f2016935@pilani.bits-pilani.ac.in>
 * 
 */ 

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include "vmx.h"

static int bitsvisor_init(void) {
    uint64_t vmxon_phy_region;
    uint64_t* vmxon_region;

    printk(KERN_INFO "[BITSV] Starting BITSVISOR...\n");

    /* Check for VMX support */
    if (vmx_support()) {
        printk(KERN_INFO "[BITSV] VMX is supported on this CPU\n");
    } else {
        printk(KERN_INFO "[BITSV] VMX is NOT supported on this CPU\n");
        return 0;
    }

    if (!set_vmx_op(&vmxon_phy_region)) {
        printk(KERN_INFO "[BITSV] VMXON failed!");
        return 0;
    } else {
        printk(KERN_INFO "[BITSV] VMXON succeeded!");
    }

    vmxon_region = __va(vmxon_phy_region);

    /* Exit VMX mode */
    __vmxoff();

    printk(KERN_INFO "[BITSV] VMX mode exited");

    /* Free the VMXON region */
    __free_vmxon_region(vmxon_region);

    printk(KERN_INFO "[BITSV] Freed 4kB memory allocated for VMXON region");
    return 0;
}

static void bitsvisor_exit(void) {
    printk(KERN_INFO "[BITSV] Exiting BITSVISOR...\n");
    return;
}

module_init(bitsvisor_init);
module_exit(bitsvisor_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Shubham Tiwari");
MODULE_DESCRIPTION("BITSVISOR: Experimental hypervisor");