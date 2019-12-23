int __alloc_vmcs_region(uint64_t* vmcs_region);

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
 * VMREAD - Read from the Virtual Machine Control Structure (VMCS)
 * In VMX root operation, the instruction reads from the current VMCS
 * If executed in VMX non-root operation, the instruction reads from the VMCS referenced by VMCS 
 * link pointer field in the current VMCS.
 * 
 * VMCS field is specified by the VMCS-field encoding 
 */ 

static inline int vmread(uint64_t encoding, uint64_t *value)
{
	uint64_t tmp;
	uint8_t ret;
	/*
	if (enable_evmcs)
		return evmcs_vmread(encoding, value);
	*/
	asm volatile("vmread %[encoding], %[value]; setna %[ret]"
		: [value]"=rm"(tmp), [ret]"=rm"(ret)
		: [encoding]"r"(encoding)
		: "cc", "memory");

	*value = tmp;
	return ret;
}

/**
 * VMWRITE - write to the Vitual Machine Control Structure (VMCS)
 * In VMX root operation, the instruction writes to the current VMCS
 * If executed in VMX non-root operation, the instruction writes to the VMCS referenced by VMCS 
 * link pointer field in the current VMCS.
 * 
 * VMCS field is specified by the VMCS-field encoding 
 */ 

static inline int vmwrite(uint64_t encoding, uint64_t value)
{
	uint8_t ret;
	asm volatile ("vmwrite %[value], %[encoding]; setna %[ret]"
		: [ret]"=rm"(ret)
		: [value]"rm"(value), [encoding]"r"(encoding)
		: "cc", "memory");

	return ret;
}

/*
 * A wrapper around vmread (taken from kvm vmx.c) that ignores errors
 * and returns zero if the vmread instruction fails.
 */

static inline uint64_t vmreadz(uint64_t encoding)
{
	uint64_t value = 0;
	vmread(encoding, &value);
	return value;
}