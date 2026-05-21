# Multiboot 1 standard: (section 3.1 - OS image format: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html)
# Multiboot 2 has some standards, including ACPI tables, so we could switch and use MB2 to parse those later.
# for now, just a bare bones implementation is enough - importantly, we set the graphics mode here (very difficult in protected
# mode later)

# Declare constants for the multiboot header.
.set F_ALIGN,    1<<0							# align loaded modules on page boundaries
.set F_MEMINFO,  1<<1							# provide memory map
.set F_GFX,		 1<<2							# suggest graphics mode)
.set FLAGS,    F_ALIGN | F_MEMINFO | F_GFX	    # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002						# 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS)					# checksum of above, to prove we are multiboot
.set MODE_TYPE, 0								# linear graphics mode. 3.1.4 of multiboot standard (above)
.set WIDTH, 1024
.set HEIGHT, 768
.set DEPTH, 32

# Declare a header as in the Multiboot Standard. (see 3.1 of the standard above)
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.skip 20			# 12-32 if flags[16] is set. we ignore this for now. the others still have their respective offset
.long MODE_TYPE
.long WIDTH
.long HEIGHT
.long DEPTH

# per the multiboot docs, once grub passes to here,
# eax will hold the magic multiboot number, and ebx
# will hold the pointer to the multiboot_info header

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp

	push %ebx	# save the multiboot_info pointer as an argument to main

	# Call the global constructors.
	call _init

	# Transfer control to the main kernel.
	call kernel_main

	# Hang if kernel_main unexpectedly returns.
	cli
1:	hlt
	jmp 1b
.size _start, . - _start
