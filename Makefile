.PHONY: all clean test i386 x86_64

# Default target - build i386
all: i386

test:
	./build.sh

# i386 build with GRUB
i386:
	@echo "Building i386 ISO with GRUB..."
	@export ARCH=i386 && ./build.sh
	@rm -rf isodir/i386
	@mkdir -p isodir/i386/boot/grub
	@cp sysroot/boot/horizon.kernel isodir/i386/boot/horizon.kernel
	@echo 'menuentry "horizon" {' > isodir/i386/boot/grub/grub.cfg
	@echo '	multiboot /boot/horizon.kernel' >> isodir/i386/boot/grub/grub.cfg
	@echo '}' >> isodir/i386/boot/grub/grub.cfg
	@grub-mkrescue -o horizon-i386.iso isodir/i386
	@echo "i386 ISO created: horizon-i386.iso"

# x86_64 build with Limine
x86_64:
	@echo "Building x86_64 ISO with Limine..."
	@export ARCH=x86_64 && ./build.sh
	@rm -rf isodir/x86_64
	@mkdir -p isodir/x86_64/
	@cp sysroot/boot/horizon.kernel isodir/x86_64/horizon.kernel
	@cp limine.conf isodir/x86_64/
	@LIMINE_DATADIR=$$(limine --print-datadir) && \
		cp "$$LIMINE_DATADIR/limine-bios.sys" isodir/x86_64/
	@xorriso -as mkisofs \
		-b limine-bios.sys \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-o horizon-x86_64.iso \
		isodir/x86_64/
	@limine bios-install horizon-x86_64.iso
	@echo "x86_64 ISO created: horizon-x86_64.iso"

clean:
	@$(MAKE) -C libc clean
	@$(MAKE) -C kernel clean
	@rm -rf sysroot
	@rm -rf isodir
	@rm -f myos.iso horizon-i386.iso horizon-x86_64.iso
	@rm -rf *.log */*.log */*/*.log
