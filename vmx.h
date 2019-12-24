#include <linux/module.h>

#define X86_CR4_VMXE_BIT	                        13 /* enable VMX virtualization */
#define X86_CR4_VMXE		                        _BITUL(X86_CR4_VMXE_BIT)
#define FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX	(1<<2)
#define FEATURE_CONTROL_LOCKED				        (1<<0)
#define MSR_IA32_FEATURE_CONTROL                    0x0000003a

#define MSR_IA32_VMX_BASIC                          0x00000480
#define MSR_IA32_VMX_CR0_FIXED0                     0x00000486
#define MSR_IA32_VMX_CR0_FIXED1                     0x00000487
#define MSR_IA32_VMX_CR4_FIXED0                     0x00000488
#define MSR_IA32_VMX_CR4_FIXED1                     0x00000489

#define CUST_PAGE_SIZE 4096

int vmx_support(void);
void __set_cr4_bit13(void);
void __feature_ctrl_msr_set(void);
void __set_fixed_crx(void);
uint64_t __alloc_vmxon_region(uint64_t* vmxon_phy_region);
void __init_vmxon_region(uint64_t *vmxon_region);

/**
 * Get VMX revision id()
 */

static inline uint32_t vmcs_rev_id(void) {
    uint32_t ret;
    rdmsrl(MSR_IA32_VMX_BASIC, ret);
    return ret;
}

int set_vmx_op(uint64_t* vmxon_phy_region);
void __free_vmxon_region(uint64_t* vmxon_region);
void __vmxoff(void);


