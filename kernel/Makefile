DEFAULT_HOST!=../default-host.sh
HOST?=$(DEFAULT_HOST)
# convert host triplet to architecture (i686-elf -> i386, x86_64-linux-gnu -> x86_64)
HOSTARCH!=echo "$(HOST)" | grep -Eq 'i[[:digit:]]86-' && echo i386 || echo "$(HOST)" | grep -Eo '^[[:alnum:]_]*'

# if we can, we should add noexecstack option here, instead of
# defining macros all over our assembly files
CFLAGS?=-O2 -g -v
CPPFLAGS?=
LDFLAGS?=
LIBS?=

NASM?=nasm

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

ARCHDIR=arch/$(HOSTARCH)

CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -fstack-protector
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -I$(ARCHDIR) -Iinclude -Iarch -Ikernel
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

include $(ARCHDIR)/make.config

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
$(ARCHDIR)/kernel.o \
kernel/kheap.o \
kernel/drivers/serial.o \
kernel/drivers/timer.o \
kernel/drivers/keyboard/keyboard.o \
kernel/apic/rsdp.o \
kernel/apic/apic.o \
kernel/apic/madt.o \
kernel/halt.o

# these were causing issues with relocation,
# for now let's ignore them. afaik they're only useful for
# cpp constructor / deconstructors
#$(ARCHDIR)/crtbegin.o \
$(ARCHDIR)/crtend.o \
#$(ARCHDIR)/boot/crtn.o
#$(TEST_OBJS_DIR)/$(ARCHDIR)/crtend.o \
$(TEST_OBJS_DIR)/$(ARCHDIR)/boot/crtn.o \
$(ARCHDIR)/boot/crtn.o \
$(ARCHDIR)/crtend.o \
$(TEST_OBJS_DIR)/$(ARCHDIR)/crtbegin.o \

# For i386, include crtbegin.o, crtend.o, and crtn.o
# For x86_64, exclude them due to relocation issues with high memory addresses
ifeq ($(HOSTARCH),i386)
OBJS=\
$(ARCHDIR)/boot/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/boot/crtn.o \

else
OBJS=\
$(ARCHDIR)/boot/crti.o \
$(KERNEL_OBJS) \

endif

# Test objs directory
TEST_OBJS_DIR=jury/objs

TEST_KERNEL_OBJS=\
$(addprefix $(TEST_OBJS_DIR)/,$(KERNEL_ARCH_OBJS)) \
$(TEST_OBJS_DIR)/kernel/kernel.o \

TEST_OBJS=\
$(TEST_OBJS_DIR)/$(ARCHDIR)/boot/crti.o \
$(TEST_KERNEL_OBJS) \

# similarly remove from linker list
# $(ARCHDIR)/crtbegin.o

ifeq ($(HOSTARCH),i386)
LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/boot/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/boot/crtn.o \

else
LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/boot/crti.o \
$(KERNEL_OBJS) \
$(LIBS) \

endif

.PHONY: all clean install install-headers install-kernel test-objs
.SUFFIXES: .o .c .S .asm

all: horizon.kernel

# test build target - compiles objects to test/objs/ directory
test-objs: $(TEST_OBJS)

$(TEST_OBJS_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(TEST_OBJS_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(TEST_OBJS_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	if [ "$(HOSTARCH)" = "x86_64" ]; then \
		$(NASM) -f elf64 $< -o $@; \
	else \
		$(NASM) -f elf32 $< -o $@; \
	fi

$(TEST_OBJS_DIR)/$(ARCHDIR)/crtbegin.o $(TEST_OBJS_DIR)/$(ARCHDIR)/crtend.o:
	@mkdir -p $(dir $@)
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

horizon.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	echo $(CC)
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	@if [ "$(HOSTARCH)" = "i386" ]; then \
		grub-file --is-x86-multiboot horizon.kernel; \
	fi
	objdump -d horizon.kernel > horizon.kernel.dis
	objdump -t horizon.kernel > horizon.kernel.sym
	readelf -a horizon.kernel > horizon.kernel.elf

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.asm.o:
	if [ "$(HOSTARCH)" = "x86_64" ]; then \
		$(NASM) -f elf64 $< -o $@; \
	else \
		$(NASM) -f elf32 $< -o $@; \
	fi

clean:
	rm -f horizon.kernel horizon.kernel.dis horizon.kernel.sym horizon.kernel.elf
	rm -f $(OBJS) *.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o */*/*/*/*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d */*/*/*.d */*/*/*/*.d */*/*/*/*/*.d
	rm -f *.log */*.log */*/*.log
	rm -rf $(TEST_OBJS_DIR)

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: horizon.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp horizon.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
