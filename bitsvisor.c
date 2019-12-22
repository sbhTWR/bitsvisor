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
#include <asm/asm.h>
#include <linux/gfp.h>
#include <linux/slab.h>

#define FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX	(1<<2)
#define FEATURE_CONTROL_LOCKED				        (1<<0)
#define MSR_IA32_FEATURE_CONTROL                    0x0000003a

#define MSR_IA32_VMX_BASIC              0x00000480
#define MSR_IA32_VMX_CR0_FIXED0         0x00000486
#define MSR_IA32_VMX_CR0_FIXED1         0x00000487
#define MSR_IA32_VMX_CR4_FIXED0         0x00000488
#define MSR_IA32_VMX_CR4_FIXED1         0x00000489

#define CUST_PAGE_SIZE 4096

int vmx_support(void);
void __init_vmxon_region(uint64_t *vmxon_region);
static inline uint32_t vmcs_rev_id(void);

/**
 * NOTE on sizes:
 * unsigned long long is guaranteed to be at least 64 bits, regardless of the platform.  
 * unsigned long is guaranteed to be at least 32 bits. If more than 32 bits is needed, 
 * unsigned long long is recommended. With regards to the formatting, with printf:
 * "%llu" (since it is unsigned); 
 * "%lld" is for signed long long.
 */ 

/* Checks for VMX hardware extension. Uses inline assembly 
    set eax = 1 and call cpuid. Bit 5 in ecx shows 
    whether virtualization is supported or not! */

int vmx_support(void) {
    /* Note: gcc doesn't parse instructions inside asm statement.
             It is therefore important to tell gcc about the registers 
             that the asm is going to modify. GCC will probably save 
             the state of the registers to avoid unexpected results.
             (For example, ebx getting modified, which might lead to 
             system freeze: 
             https://stackoverflow.com/questions/59442341/ubuntu-freezes-completely-on-executing-this-inline-asm
             ) 
        The following asm is inspired from linux code. Could have used cpuid_eax() from <asm/processor.h> also.
        https://github.com/torvalds/linux/blob/master/arch/x86/include/asm/processor.h     
    */
    unsigned int eax, ebx, ecx, edx, vmx_bit;
    eax = 1;
    ecx = 0;
    asm volatile("cpuid"
	    : "=a" (eax),
	      "=b" (ebx),
	      "=c" (ecx),
	      "=d" (edx)
	    : "0" (eax), "2" (ecx)
	    : "memory");

    vmx_bit = (ecx >> 5) & 1;
    if (vmx_bit == 1) {
        return 1;
    } else {
        return 0;
    } 
}

/**
 *  Sets bit 13 of CR4 [TODO: Reason?] 
 */
void __set_cr4_bit13(void) {
    unsigned long cr4;
    /* set bit 13 of cr4 */
    asm volatile ("mov %%cr4, %0" : "=r"(cr4) : : "memory");
    cr4 |= X86_CR4_VMXE;
    asm volatile ("mov %0, %%cr4" : : "r"(cr4) : "memory");
}

/**
 * -- Some theory -- 
 * MSR (Model Specific Registers) - Following pentium processors
 * Intel provides a pair of instructions ('rdmsr' and 'wrmsr') to access 
 * current and future "model-specific registers" along with 'cpuid' to
 * determine which features are present on a particular processor.
 * 
 * Reading is handled by 'rdmsr'
 * Writing is handled by 'wrmsr' 
 * 
 * MSR traceing: It is a hardware feature on most modern Intel processors.
 * MSR accesses (reads and writes) can be traced. Tracepoints help to 
 * construct a system-wide picture of what is going on in the system. 
 * 
 * SMX (Safer Mode Extenstions) is a feature of Intel processors which is 
 * used to measure (generate cryptographic hash) a VMM (hypervisor) and is 
 * also used to attest (check hash) of a VMM before it is allowed to run.
 * 
 * SMX instructions provide the ability to perform an accurate measurement of 
 * the vmm. Basically, it seems SMX instructions provide protection against
 * software attack.[TODO: **More clearity needed**]
 * 
 * Linux provides primtives for reading and writing to MSRs.
 * Note: MSR are 64-bit registers. High and low represent the higher
 *       32 bits and the lower 32 bits respectively.
 * 
 * <static inline unsigned long long notrace __rdmsr(unsigned int msr)>
 * <static inline void notrace __wrmsr(unsigned int msr, u32 low, u32 high)>
 * They are defined in: <asm/msr.h>
 * https://github.com/torvalds/linux/blob/master/arch/x86/include/asm/msr.h
 * 
 * wrmsr() does a normal __wrmsr() and also writes a trace if tracepoint is active.
 * rdmsr() does a normal __rdmsr() and also writes a trace if tracepoint is active.
 * Both wrmsr() and rdmsr() allow reading and writing through 32 bit variables into msr
 * by using a pair of low and high 32 bit variables.
 * 
 * wrmsrl() and rdmsrl() allow writing u64 values directly (unsigned long long)
 * The last letter 'l' in both the function names indicates this.
 * 
 * #GP - General-Protection exception.
 * 
 * -- Enabling VMXON operation --
 * VMXON is controlled by IA32_FEATURE_CONTROL MSR (MSR address 3AH). 
 * [Bit 0 is the lock bit]: if clear, VMXON causes #GP  
 *                          if set, wrmsr to this MSR causes #GP; MSR 
 * cannot be modified until a power-up reset condition.
 * [Bit 1 enables VMXON in SMX operation]: if clear, VMXON in SMX causes #GP
 * [Bit 2 enables VMXON outside SMX operation]: if clear, VMXON outside SMX causes #GP  
 * 
 * Since we are going to execute VMXON outside SMX, Bit 0 and Bit 2 of MSR neet to be set. 
 */

void __feature_ctrl_msr_set(void) {

    uint64_t feature_ctrl;
    uint64_t req;
    req = FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX 
          | FEATURE_CONTROL_LOCKED;

    rdmsrl(MSR_IA32_FEATURE_CONTROL, feature_ctrl);
    printk(KERN_INFO "[BITSV] rdmsr: %ld", (long)feature_ctrl);

    if ((feature_ctrl & req) != req) {
        wrmsrl(MSR_IA32_FEATURE_CONTROL, feature_ctrl | req);
    }
}

/**
 * Processors may fix some bits CR0 and CR4 in VMX operation, and not allow
 * any other value. VMXON will fail if these bits have any other values in 
 * the CR4 and CR0 register. Also, while in VMXON operation, any attempt to 
 * change these values to unsupported values causes #GP
 * 
 * To execute VMXON, it is required to read the the fixed bits configuration
 * required from the MSRs and set/clear them in CR0 and CR4 accordingly before 
 * executing VMXON.
 * 
 * _FIXED0 - enforces 1 in certain bit positions (so or (|) operation on CRX)
 * _FIXED1 - enforces 0 in certain bit positions (so and (&) operation on CRX)
 * 
 */

void __set_fixed_crx(void) {
    unsigned long cr0, cr4; /* guaranteed to be at least 32 bit (TODO: why not use uint64_t?) */
    uint64_t val;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0) : : "memory");
    rdmsrl(MSR_IA32_VMX_CR0_FIXED1, val);
    cr0 &= val;
    rdmsrl(MSR_IA32_VMX_CR0_FIXED0, val);
    cr0 |= val;
    asm volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");

    asm volatile ("mov %%cr4, %0" : "=r"(cr4) : : "memory");
    rdmsrl(MSR_IA32_VMX_CR4_FIXED1, val);
    cr4 &= val;
    rdmsrl(MSR_IA32_VMX_CR4_FIXED0, val);
    cr4 |= val;
    asm volatile ("mov %0, %%cr4" : : "r"(cr4) : "memory");
}

/**
 * Allocate VMXON region which is a 4kB aligned memory, used
 * by the logical processor to support VMX operation. The pointer
 * to region is supplied as an operand to VMXON. The VMXON pointer
 * is subject to the following constraints:
 * 
 * 1. 4kB aligned, therefore bits 11:0 must be zero.
 * 2. The VMXON region must not set any bits beyond the processors physical 
 *    address width. 
 * 
 * NOTE: __pa() used to map virtual address to physical address allocated
 * using kzalloc() in the kernel memory. Since the virtual to physical
 * address mapping in the kernel memory and direct and static, it simply
 * differs by an offset which is architecture dependent. This converted 
 * physical address (which would be identical that that obtained by a page
 * walk done by MMU on hardware) is only used for references inside the kernel
 * code and not actually used for accessing memory (as that will be handled 
 * by MMU on the hardware as usual).
 */ 

uint64_t __alloc_vmxon_region(void) {
    uint64_t *vmxon_region;
    uint64_t vmxon_phy_region ;

    vmxon_region = kzalloc(CUST_PAGE_SIZE, GFP_KERNEL);
    if (!vmxon_region) {
        printk(KERN_INFO "[BITSV] Error allocating VMXON region");
        return 0;
    }

    vmxon_phy_region = __pa(vmxon_region);
    printk(KERN_INFO "[BITSV] VMXON region physical address: %lld", (unsigned long long)vmxon_phy_region);
    /* init the VMXON region */
    __init_vmxon_region(vmxon_region);

    return vmxon_phy_region;
}

/**
 * Intialize VMXON region by writing first 32 bits in the following way:
 * bit 31 - clear to zero
 * bits 30:0 - write the 31 bit revision identifier obtained from MSR.
 * 
 * If the revision identifier does not match the one used by the processor
 * then VMXON fails.
 */

void __init_vmxon_region(uint64_t *vmxon_region) {
    *(uint32_t *)vmxon_region = vmcs_rev_id(); 
}

/**
 * Get VMX revision id()
 */

static inline uint32_t vmcs_rev_id(void) {
    uint32_t ret;
    rdmsrl(MSR_IA32_VMX_BASIC, ret);
    return ret;
}

/**
 * VMXON instruction - Enter VMX operation
 */ 

static inline int __vmxon(uint64_t phya) {
    uint8_t ret;
    asm volatile ("vmxon %[pa]; setna %[ret]"
                  : [ret]"=rm"(ret)
                  : [pa]"m"(phya)
                  : "cc", "memory");
    return ret;              
}

/**
 * Allocate VMXON region and execute VMXON
 */ 

int set_vmx_op(void) {
    uint64_t phya;
    __set_cr4_bit13();
    __feature_ctrl_msr_set();
    __set_fixed_crx();

    phya = __alloc_vmxon_region();
    if (!phya) return 0;

    if(__vmxon(phya)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Execute VMXOFF 
 */

void __vmxoff(void) {
    asm volatile ("vmxoff\n" : : : "cc");
}

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