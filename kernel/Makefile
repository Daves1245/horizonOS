DEFAULT_HOST!=../default-host.sh
HOST?=$(DEFAULT_HOST)
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

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
kernel/kernel.o \
kernel/kheap.o \
kernel/jury/test_paging.o \
kernel/jury/test_vm.o \

OBJS=\
$(ARCHDIR)/boot/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/boot/crtn.o \

# Test objs directory
TEST_OBJS_DIR=jury/objs

TEST_KERNEL_OBJS=\
$(addprefix $(TEST_OBJS_DIR)/,$(KERNEL_ARCH_OBJS)) \
$(TEST_OBJS_DIR)/kernel/kernel.o \

TEST_OBJS=\
$(TEST_OBJS_DIR)/$(ARCHDIR)/boot/crti.o \
$(TEST_OBJS_DIR)/$(ARCHDIR)/crtbegin.o \
$(TEST_KERNEL_OBJS) \
$(TEST_OBJS_DIR)/$(ARCHDIR)/crtend.o \
$(TEST_OBJS_DIR)/$(ARCHDIR)/boot/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
$(ARCHDIR)/boot/crti.o \
$(ARCHDIR)/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/boot/crtn.o \

.PHONY: all clean install install-headers install-kernel test-objs
.SUFFIXES: .o .c .S .asm

all: myos.kernel

# Test build target - compiles objects to test/objs/ directory
test-objs: $(TEST_OBJS)

$(TEST_OBJS_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(TEST_OBJS_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(TEST_OBJS_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) -f elf32 $< -o $@

$(TEST_OBJS_DIR)/$(ARCHDIR)/crtbegin.o $(TEST_OBJS_DIR)/$(ARCHDIR)/crtend.o:
	@mkdir -p $(dir $@)
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

myos.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	echo $(CC)
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot myos.kernel
	objdump -d myos.kernel > myos.kernel.dis
	objdump -t myos.kernel > myos.kernel.sym
	readelf -a myos.kernel > myos.kernel.elf

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.S.o:
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

.asm.o:
	$(NASM) -f elf32 $< -o $@

clean:
	rm -f myos.kernel myos.kernel.dis myos.kernel.sym myos.kernel.elf
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm -rf $(TEST_OBJS_DIR)

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: myos.kernel
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp myos.kernel $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
