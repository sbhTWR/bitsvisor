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
#include "vmx.h"

static int bitsvisor_init(void) {
    printk(KERN_INFO "[BITSV] Starting BITSVISOR...\n");

    /* Check for VMX support */
    if (vmx_support()) {
        printk(KERN_INFO "[BITSV] VMX is supported on this CPU\n");
    } else {
        printk(KERN_INFO "[BITSV] VMX is NOT supported on this CPU\n");
        return 0;
    }

    if (!set_vmx_op()) {
        printk(KERN_INFO "[BITSV] VMXON failed!");
        return 0;
    } else {
        printk(KERN_INFO "[BITSV] VMXON succeeded!");
    }

    /* Exit VMX mode */
    __vmxoff();
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