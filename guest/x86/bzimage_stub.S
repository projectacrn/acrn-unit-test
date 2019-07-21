/*
 * Stub code that serves as the entry of a bzimage to load a unit test image.
 */

#define MEM_1K             0x400
#define MEM_2K             (2 * MEM_1K)
#define MEM_4K             (4 * MEM_1K)
#define MEM_1M             (1024 * MEM_1K)
#define SECTOR_SIZE        (MEM_1K / 2)

#define ZEROPAGE_MAGIC_SIG    0x53726448    /* "HdrS" in uint32_t */
#define ZEROPAGE_SETUP_SECTS  0x1f1
#define ZEROPAGE_HEADER       0x202
#define ZEROPAGE_CMDLINE_PTR  0x228
#define ZEROPAGE_IMAGE_SIZE   0x3fc
#define ZEROPAGE_BINARY_BASE  (4 * SECTOR_SIZE)

#define MBI_FLAGS             0x0
#define MBI_MEM_LOWER         0x4
#define MBI_MEM_UPPER         0x8
#define MBI_CMDLINE           0x10
#define MBI_MODS_COUNT        0x14

#define MULTIBOOT_HEADER_MAGIC          0x1badb002
#define MULTIBOOT_INFO_MAGIC		0x2badb002
#define MULTIBOOT_INFO_HAS_MEMORY	0x00000001
#define MULTIBOOT_INFO_HAS_BOOT_DEVICE	0x00000002
#define MULTIBOOT_INFO_HAS_CMDLINE	0x00000004
#define MULTIBOOT_INFO_HAS_MODS		0x00000008
#define MULTIBOOT_INFO_HAS_AOUT_SYMS	0x00000010
#define MULTIBOOT_INFO_HAS_ELF_SYMS	0x00000020
#define MULTIBOOT_INFO_HAS_MMAP		0x00000040
#define MULTIBOOT_INFO_HAS_DRIVES	0x00000080
#define MULTIBOOT_INFO_HAS_CONFIG_TABLE	0x00000100
#define MULTIBOOT_INFO_HAS_LOADER_NAME	0x00000200
#define MULTIBOOT_INFO_HAS_APM_TABLE	0x00000400
#define MULTIBOOT_INFO_HAS_VBE		0x00000800

#define UNIT_TEST_LOAD_ADDRESS     (4 * MEM_1M)
#define MBI_ADDRESS                (UNIT_TEST_LOAD_ADDRESS - MEM_4K)
#define CMDLINE_ADDRESS            (UNIT_TEST_LOAD_ADDRESS - MEM_2K)

#define UART_BASE          0x3f8
#define UART_RBR           0x00
#define UART_THR           0x00
#define UART_DLL           0x00
#define UART_IER           0x01
#define UART_DLM           0x01
#define UART_IIR           0x02
#define UART_FCR           0x02
#define UART_LCR           0x03
#define UART_MCR           0x04
#define UART_LSR           0x05
#define UART_MSR           0x06
#define UART_SCR           0x07

	/* Register allocation
	 *
	 *     EAX: General, used for IN/OUT and as temporary variables
	 *     EBX: General, used as a temporary variable
	 *     ECX: General, used as iterators
	 *     EDX: Used for IN/OUT when initializing UART and as the zero page pointer afterwards
	 *     EBP: Used as the base of this stub before jumping to the trampoline, and bzimage pointer afterwards
	 *     ESI: Used as the source iterator during copying
	 *     EDI: Used as the destination iterator during copying
	 */

	.code32

	/* Entry of the stub. Shall be the third sector in the bzimage. */
start:
	mov	$0xa0000, %esp

	/* Init UART FIFOs */
	mov	$(UART_BASE + UART_FCR), %dx
	mov	$0x7, %al
	outb	%al, %dx

	/* Init UART data bits / parity bits / stop bits */
	mov	$(UART_BASE + UART_LCR), %dx
	mov	$0x3, %al
	outb	%al, %dx

	/* Disable UART interrupts */
	mov	$(UART_BASE + UART_IER), %dx
	mov	$0, %al
	outb	%al, %dx

	/* Set UART terminal ready and request to send */
	mov	$(UART_BASE + UART_MCR), %dx
	mov	$3, %al
	outb	%al, %dx

	mov	%esi, %edx

	/* Copy trampoline code to the physical address 0. This is the place
	 * where acrn-unit-test uses as the trampoline for starting APs as well. So
	 * RAM at this address shall exists nonetheless.
	 */

	/* Identify base of the stub */
	call	1f
1:
	pop	%ebp
	and	$(~(SECTOR_SIZE-1)), %ebp

	mov	%ebp, %esi
	add	$(trampoline_start - start), %esi
	xor	%edi, %edi
	mov	$(trampoline_end - trampoline_start), %ecx
	rep/movsb

	xor	%eax, %eax
	jmp	*%eax

trampoline_start:

	/* Convert the base of this stub to the base of the bzimage. */

	mov	ZEROPAGE_SETUP_SECTS(%edx), %eax
	add	$1, %eax
	shl	$9, %eax       /* (x << 9) == (x * SECTOR_SIZE) */
	sub	%eax, %ebp

	/* Copy the unit test image to 4M. */

	lea	ZEROPAGE_BINARY_BASE(%ebp), %esi
	mov	$(UNIT_TEST_LOAD_ADDRESS), %edi
	mov	ZEROPAGE_IMAGE_SIZE(%ebp), %ecx

	/* If the address 4M locates in the range of the unit test image, copy
	 * in the reversed order by setting EFLAGS.DF. */

	cld
	cmp	$(UNIT_TEST_LOAD_ADDRESS), %esi
	jge	1f
	mov	%esi, %eax
	add	%ecx, %eax
	cmp	$(UNIT_TEST_LOAD_ADDRESS), %eax
	jl	1f

	add	%ecx, %esi
	sub	$1, %esi
	add	%ecx, %edi
	sub	$1, %edi
	std

1:
	rep/movsb
	cld

	/* Construct a simple multiboot header for the unit test. */

	mov	$MBI_ADDRESS, %esi

	mov	$(MULTIBOOT_INFO_HAS_MEMORY | MULTIBOOT_INFO_HAS_CMDLINE | MULTIBOOT_INFO_HAS_MODS), %eax
	mov	%eax, MBI_FLAGS(%esi)

	mov	$0, %eax
	mov	%eax, MBI_MEM_LOWER(%esi)
	mov	%eax, MBI_MODS_COUNT(%esi)

	/* TODO: parse the end of memory from e820 */
	mov	$(MEM_1M / MEM_1K * 512), %eax
	mov	%eax, MBI_MEM_UPPER(%esi)

	mov	$(CMDLINE_ADDRESS), %eax
	mov	%eax, MBI_CMDLINE(%esi)

	mov	ZEROPAGE_CMDLINE_PTR(%edx), %esi
	mov	$(CMDLINE_ADDRESS), %edi
	mov	$(MEM_2K), %ecx
	repnz/movsb

	/* Identify the entry which is right after the multiboot header. */

	mov	0, %ecx
	mov	$(UNIT_TEST_LOAD_ADDRESS), %esi
1:
	mov	0(%esi), %eax
	cmp	$(MULTIBOOT_HEADER_MAGIC), %eax
	jz	_call_entry
	add	$4, %ecx
	add	$4, %esi
	cmp	$(8 * MEM_1K), %ecx
	jnz	1b

	/* Prepare for the defined register states and Jump to the entry. */
_call_entry:

	/* The multiboot header is 12 bytes, followed by the entry point. */

	add	$12, %esi
	mov	$MULTIBOOT_INFO_MAGIC, %eax
	mov	$MBI_ADDRESS, %ebx
	jmp	*%esi

print_edi:
	push	%eax
	push	%ebx
	push	%ecx
	push	%edx

	mov	$32, %cl

1:
	cmp	$0, %cl
	jz	_print_edi_end

	sub	$4, %cl
	mov	%edi, %ebx
	shr	%cl, %ebx
	andb	$0xf, %bl
	addb	$0x30, %bl
	cmp	$0x39, %bl
	jle	2f
	addb	$(0x61-0x3a), %bl
2:

	mov	$(UART_BASE + UART_LSR), %dx
3:
	inb	%dx, %al
	andb	$0x60, %al
	cmp	$0x60, %al
	jnz	3b

	mov	%bl, %al
	mov	$(UART_BASE + UART_THR), %dx
	outb	%al, %dx
	jmp	1b

_print_edi_end:
	/* Print a newline at the end */
	mov	$0x0d, %al
	outb	%al, %dx
	mov	$0x0a, %al
	outb	%al, %dx

	pop	%edx
	pop	%ecx
	pop	%ebx
	pop	%eax

	ret

trampoline_end:
