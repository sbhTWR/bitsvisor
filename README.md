# bitsvisor
Kernel Module that enables Linux to act as a VMM. Leverges x86's VT-x to support virtualization. 
The motivation behind the project is to learn about virtualization and how hypervisors work by building a minimal hypervisor. The code in the project is inspired from KVM and an online [tutorial](http://www.nixhacker.com/developing-hypervisior-from-scratch-part-1/). Learning experience from the project till now:
* Inline assembly (importance of extended assembly, which resulted in a [stackoverflow question]())
* How to access registers (such as CR4) using inline assembly.
* VT-x instructions, MSRs and how to access them.
* How to allocate kernel memory and importane of `__pa()` (virtual to physical address translation in kernel)

### Features
Not much yet. Can enter VMX mode and return from it as of now.

### TODO
* Using vmread and vmwrite, set different control fields in VMCS for:

    -> VM-entry
    
    -> VM-execution
    
    -> VM-exit 
    
 * Initialize guest and the host state area in VCMS:
 
     -> define functions for accessing CR3 and CR4 registers.
     
     -> set various segment registers values.
 * Define functions for freeing allocated VMCS and VMXON regions using kfree()

