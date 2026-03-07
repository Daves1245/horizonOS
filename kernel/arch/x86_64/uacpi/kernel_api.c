/*
 * uACPI Kernel API Implementation for Horizon OS
 *
 * This file implements the kernel abstraction layer required by uACPI.
 * See: https://github.com/uACPI/uACPI/blob/master/include/uacpi/kernel_api.h
 */

#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/log.h>
#include <uacpi/platform/arch_helpers.h>
#include <uacpi/kernel_api.h>
#include <apic/rsdp.h>
#include <x86_64/memory/paging.h>
#include <kernel/logger.h>
#include <kheap.h>
#include <limine.h>
#include <drivers/io.h>
#include <stdint.h>

extern volatile struct limine_hhdm_request hhdm_request;

/*
 * ============================================================================
 * Early/Barebones Mode APIs
 * These functions are available in both barebones and full mode.
 * ============================================================================
 */

/*
 * Returns the PHYSICAL address of the RSDP structure via *out_rsdp_address.
 */
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    *out_rsdp_address = get_rsdp_phys();
    return UACPI_STATUS_OK;
}

/*
 * Map a physical memory range starting at 'addr' with length 'len', and return
 * a virtual address that can be used to access it.
 *
 * NOTE: 'addr' may be misaligned, in this case the host is expected to round it
 *       down to the nearest page-aligned boundary and map that, while making
 *       sure that at least 'len' bytes are still mapped starting at 'addr'. The
 *       return value preserves the misaligned offset.
 *
 *       Example for uacpi_kernel_map(0x1ABC, 0xF00):
 *           1. Round down the 'addr' we got to the nearest page boundary.
 *              Considering a PAGE_SIZE of 4096 (or 0x1000), 0x1ABC rounded down
 *              is 0x1000, offset within the page is 0x1ABC - 0x1000 => 0xABC
 *           2. Requested 'len' is 0xF00 bytes, but we just rounded the address
 *              down by 0xABC bytes, so add those on top. 0xF00 + 0xABC => 0x19BC
 *           3. Round up the final 'len' to the nearest PAGE_SIZE boundary, in
 *              this case 0x19BC is 0x2000 bytes (2 pages if PAGE_SIZE is 4096)
 *           4. Call the VMM to map the aligned address 0x1000 (from step 1)
 *              with length 0x2000 (from step 3). Let's assume the returned
 *              virtual address for the mapping is 0xF000.
 *           5. Add the original offset within page 0xABC (from step 1) to the
 *              resulting virtual address 0xF000 + 0xABC => 0xFABC. Return it
 *              to uACPI.
 */
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    // map_physical_range() will handle alignment internally, so just use as is
    map_physical_range(addr, len, 1, 1);

    // we have the physical range, with Limine we're mapped
    // into HHDM so just add the offset to get the virtual address
    uint64_t hhdm_offset = hhdm_request.response->offset;
    void *virt_addr = (void *)(addr + hhdm_offset);

    return virt_addr;
}

/*
 * Unmap a virtual memory range at 'addr' with a length of 'len' bytes.
 *
 * NOTE: 'addr' may be misaligned, see the comment above 'uacpi_kernel_map'.
 *       Similar steps to uacpi_kernel_map can be taken to retrieve the
 *       virtual address originally returned by the VMM for this mapping
 *       as well as its true length.
 */
void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    // for now, we don't actually unmap since we're using HHDM which is
    // a direct physical memory mapping
    (void)addr;
    (void)len;
}

/*
 * Logging callback for uACPI.
 * In barebones mode, this receives pre-formatted strings.
 */
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *msg) {
    // Map uACPI log levels to your kernel's log levels
    switch (level) {
        case UACPI_LOG_DEBUG:
            log_debug("[uACPI] %s", msg);
            break;
        case UACPI_LOG_INFO:
            log_info("[uACPI] %s", msg);
            break;
        case UACPI_LOG_WARN:
            log_warning("[uACPI] %s", msg);
            break;
        case UACPI_LOG_ERROR:
            log_error("[uACPI] %s", msg);
            break;
        default:
            log_info("[uACPI] %s", msg);
            break;
    }
}

#ifndef UACPI_BAREBONES_MODE

/*
 * ============================================================================
 * Full Mode APIs
 * These functions are only required when not using barebones mode.
 * ============================================================================
 */

/*
 * Allocate a block of memory of 'size' bytes.
 * The contents of the allocated memory are unspecified.
 */
void *uacpi_kernel_alloc(uacpi_size size) {
    return (void *) kmalloc((uint32_t) size);
}

/*
 * Free a previously allocated memory block.
 *
 * 'mem' might be a NULL pointer. In this case, the call is assumed to be a
 * no-op.
 *
 * An optionally enabled 'size_hint' parameter contains the size of the original
 * allocation. Note that in some scenarios this incurs additional cost to
 * calculate the object size.
 */
void uacpi_kernel_free(void *mem) {
    // since we use a bump allocator for now, this is a no-op
    (void) mem;
}

/*
 * ============================================================================
 * PCI Configuration Space Access
 * ============================================================================
 */

/*
 * Open a PCI device at 'address' for reading & writing.
 *
 * Note that this must be able to open any arbitrary PCI device, not just those
 * detected during kernel PCI enumeration, since the following pattern is
 * relatively common in AML firmware:
 *    Device (THC0)
 *    {
 *        // Device at 00:10.06
 *        Name (_ADR, 0x00100006)  // _ADR: Address
 *
 *        OperationRegion (THCR, PCI_Config, Zero, 0x0100)
 *        Field (THCR, ByteAcc, NoLock, Preserve)
 *        {
 *            // Vendor ID field in the PCI configuration space
 *            VDID,   32
 *        }
 *
 *        // Check if the device at 00:10.06 actually exists, that is reading
 *        // from its configuration space returns something other than 0xFFs.
 *        If ((VDID != 0xFFFFFFFF))
 *        {
 *            // Actually create the rest of the device's body if it's present
 *            // in the system, otherwise skip it.
 *        }
 *    }
 *
 * The handle returned via 'out_handle' is used to perform IO on the
 * configuration space of the device.
 */
uacpi_status uacpi_kernel_pci_device_open(uacpi_pci_address addr, uacpi_handle *out_handle) {
    (void) addr;
    (void) out_handle;
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    (void)handle;
}

/*
 * Read & write the configuration space of a previously open PCI device.
 */
uacpi_status uacpi_kernel_pci_read8(uacpi_handle device, uacpi_size offset, uacpi_u8 *out_value) {
    (void) device;
    (void) offset;
    (void) out_value;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_read16(uacpi_handle device, uacpi_size offset, uacpi_u16 *out_value) {
    (void) device;
    (void) offset;
    (void) out_value;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_read32(uacpi_handle device, uacpi_size offset, uacpi_u32 *out_value) {
    (void) device;
    (void) offset;
    (void) out_value;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write8(uacpi_handle device, uacpi_size offset, uacpi_u8 value) {
    (void) device;
    (void) offset;
    (void) value;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write16(uacpi_handle device, uacpi_size offset, uacpi_u16 value) {
    (void) device;
    (void) offset;
    (void) value;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_pci_write32(uacpi_handle device, uacpi_size offset, uacpi_u32 value) {
    (void) device;
    (void) offset;
    (void) value;
    return UACPI_STATUS_UNIMPLEMENTED;
}

/*
 * ============================================================================
 * SystemIO Address Space Access
 * ============================================================================
 */

/*
 * Map a SystemIO address at [base, base + len) and return a kernel-implemented
 * handle that can be used for reading and writing the IO range.
 *
 * NOTE: The x86 architecture uses the in/out family of instructions
 *       to access the SystemIO address space.
 */
uacpi_status uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle) {
    // on x86/x86_64, I/O ports don't need mapping like memory does.
    // we just store the base port address as the handle.
    (void)len;  // length is unused, but validated by uacpi
    *out_handle = (uacpi_handle)(uintptr_t)base;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
    (void) handle;
}

/*
 * Read/Write the IO range mapped via uacpi_kernel_io_map
 * at a 0-based 'offset' within the range.
 *
 * NOTE:
 * The x86 architecture uses the in/out family of instructions
 * to access the SystemIO address space.
 *
 * You are NOT allowed to break e.g. a 4-byte access into four 1-byte accesses.
 * Hardware ALWAYS expects accesses to be of the exact width.
 */
uacpi_status uacpi_kernel_io_read8(uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value) {
    uacpi_io_addr port = (uacpi_io_addr)(uintptr_t)handle + offset;
    *out_value = inb((uint16_t)port);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value) {
    uacpi_io_addr port = (uacpi_io_addr)(uintptr_t)handle + offset;
    *out_value = inw((uint16_t)port);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value) {
    uacpi_io_addr port = (uacpi_io_addr)(uintptr_t)handle + offset;
    *out_value = inl((uint16_t)port);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(uacpi_handle handle, uacpi_size offset, uacpi_u8 value) {
    uacpi_io_addr port = (uacpi_io_addr)(uintptr_t)handle + offset;
    outb((uint16_t)port, value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(uacpi_handle handle, uacpi_size offset, uacpi_u16 value) {
    uacpi_io_addr port = (uacpi_io_addr)(uintptr_t)handle + offset;
    outw((uint16_t)port, value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(uacpi_handle handle, uacpi_size offset, uacpi_u32 value) {
    uacpi_io_addr port = (uacpi_io_addr)(uintptr_t)handle + offset;
    outl((uint16_t)port, value);
    return UACPI_STATUS_OK;
}

/*
 * ============================================================================
 * Timing and Sleep
 * ============================================================================
 */

/*
 * read the Time Stamp Counter (TSC)
 *
 * https://wiki.osdev.org/TSC
 * https://aakinshin.net/vignettes/tsc/
 */
static inline uint64_t rdtsc(void) {
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t) high << 32) | low;
}

/*
 * get TSC frequency in Hz using cpuid (if available)
 * returns 0 if not supported
 */
static uint64_t get_tsc_frequency_cpuid(void) {
    uint32_t eax, ebx, ecx, edx;

    // check if cpuid leaf 0x15 (tsc/crystal clock info) is available
    asm volatile("cpuid" : "=a"(eax) : "a"(0) : "ebx", "ecx", "edx");
    if (eax < 0x15) {
        return 0;
    }

    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x15));

    // not supported
    if (!eax || !ebx) {
        return 0;
    }

    if (ecx != 0) {
        // tsc frequency = (crystal_hz * ebx) / eax
        return ((uint64_t) ecx * ebx) / eax;
    }

    return 0;  // need to calibrate
}

/*
 * Returns the number of nanosecond ticks elapsed since boot,
 * strictly monotonic.
 */
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    static uint64_t boot_tsc = 0;
    static uint64_t tsc_frequency = 0;

    // initialize on first call
    if (boot_tsc == 0) {
        boot_tsc = rdtsc();

        tsc_frequency = get_tsc_frequency_cpuid();

        // for fallback, we assume 2ghz (common for qemu)
        if (tsc_frequency == 0) {
            tsc_frequency = 2000000000ULL;  // 2 ghz
            log_warning("[uacpi timing]: tsc frequency unknown, assuming 2 GHz");
        } else {
            log_info("[uacpi timing]: tsc frequency: %llu hz", tsc_frequency);
        }
    }

    uint64_t current_tsc = rdtsc();
    uint64_t elapsed_cycles = current_tsc - boot_tsc;

    // cycles to nanoseconds: (cycles * 1'000'000'000) / freq
    return (elapsed_cycles * 1000000000ULL) / tsc_frequency;
}

/*
 * Spin for N microseconds.
 */
void uacpi_kernel_stall(uacpi_u8 usec) {
    uint64_t start = uacpi_kernel_get_nanoseconds_since_boot();
    uint64_t target = start + (usec * 1000ULL);  // micro -> nano

    // busy wait
    while (uacpi_kernel_get_nanoseconds_since_boot() < target) {
        asm volatile("pause");  // hint to the processor that we're spinning
    }
}

/*
 * Sleep for N milliseconds.
 */
void uacpi_kernel_sleep(uacpi_u64 msec) {
    // TODO in the future, this should yield to a scheduler
    uint64_t start = uacpi_kernel_get_nanoseconds_since_boot();
    uint64_t target = start + (msec * 1000000ULL);  // milli -> nano

    while (uacpi_kernel_get_nanoseconds_since_boot() < target) {
        asm volatile("pause"); // suggest we're in a spin again
    }
}

/*
 * ============================================================================
 * Synchronization Primitives
 * ============================================================================
 */

/*
 * Create/free an opaque non-recursive kernel mutex object.
 */
uacpi_handle uacpi_kernel_create_mutex(void) {
    // TODO implement mutex creation
    return (uacpi_handle) 1;
}

void uacpi_kernel_free_mutex(uacpi_handle handle) {
    (void) handle;
}

/*
 * Try to acquire the mutex with a millisecond timeout.
 *
 * The timeout value has the following meanings:
 * 0x0000 - Attempt to acquire the mutex once, in a non-blocking manner
 * 0x0001...0xFFFE - Attempt to acquire the mutex for at least 'timeout'
 *                   milliseconds
 * 0xFFFF - Infinite wait, block until the mutex is acquired
 *
 * The following are possible return values:
 * 1. UACPI_STATUS_OK - successful acquire operation
 * 2. UACPI_STATUS_TIMEOUT - timeout reached while attempting to acquire (or the
 *                           single attempt to acquire was not successful for
 *                           calls with timeout=0)
 * 3. Any other value - signifies a host internal error and is treated as such
 */
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {
    (void) handle;
    (void) timeout;
    return UACPI_STATUS_OK; // single-threaded for now
}

void uacpi_kernel_release_mutex(uacpi_handle handle) {
    (void) handle;
}

/*
 * Create/free an opaque kernel (semaphore-like) event object.
 */
uacpi_handle uacpi_kernel_create_event(void) {
    // TODO implement event creation
    return (uacpi_handle) 1; // return non-null to indicate success
}

void uacpi_kernel_free_event(uacpi_handle handle) {
    (void) handle;
}

/*
 * Try to wait for an event (counter > 0) with a millisecond timeout.
 * A timeout value of 0xFFFF implies infinite wait.
 *
 * The internal counter is decremented by 1 if wait was successful.
 *
 * A successful wait is indicated by returning UACPI_TRUE.
 */
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
    (void) handle;
    (void) timeout;
    return UACPI_TRUE; // assume event is signaled for now
}

/*
 * Signal the event object by incrementing its internal counter by 1.
 *
 * This function may be used in interrupt contexts.
 */
void uacpi_kernel_signal_event(uacpi_handle handle) {
    (void) handle;
}

/*
 * Reset the event counter to 0.
 */
void uacpi_kernel_reset_event(uacpi_handle handle) {
    (void) handle;
}

/*
 * Returns a unique identifier of the currently executing thread.
 *
 * The returned thread id cannot be UACPI_THREAD_ID_NONE.
 */
uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    // TODO implement thread ID
    return (uacpi_thread_id) 1; // single-threaded for now
}

/*
 * ============================================================================
 * Spinlocks (Interrupt-Safe Locks)
 * ============================================================================
 */

/*
 * Create/free a kernel spinlock object.
 *
 * Unlike other types of locks, spinlocks may be used in interrupt contexts.
 */
uacpi_handle uacpi_kernel_create_spinlock(void) {
    return (uacpi_handle) 1;
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
    (void) handle;
}

/*
 * Lock/unlock helpers for spinlocks.
 *
 * These are expected to disable interrupts, returning the previous state of cpu
 * flags, that can be used to possibly re-enable interrupts if they were enabled
 * before.
 *
 * Note that lock is infallible.
 */
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
    (void) handle;
    return 0;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
    (void) handle;
    (void) flags;
}

/*
 * ============================================================================
 * Interrupt Handling
 * ============================================================================
 */

/*
 * Install an interrupt handler at 'irq', 'ctx' is passed to the provided
 * handler for every invocation.
 *
 * 'out_irq_handle' is set to a kernel-implemented value that can be used to
 * refer to this handler from other API.
 */
uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
) {
    // TODO: wire SCI/GPE IRQ through IOAPIC and register with ISR
    (void) irq;
    (void) handler;
    (void) ctx;
    *out_irq_handle = (uacpi_handle) 1;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, uacpi_handle irq_handle
) {
    (void) handler;
    (void) irq_handle;
    return UACPI_STATUS_OK;
}

/*
 * ============================================================================
 * Deferred Work Execution
 * ============================================================================
 */

/*
 * Schedules deferred work for execution.
 * Might be invoked from an interrupt context.
 *
 * Work types:
 * - UACPI_WORK_GPE_EXECUTION: Schedule a GPE handler method for execution.
 *                             This should be scheduled to run on CPU0 to avoid
 *                             potential SMI-related firmware bugs.
 * - UACPI_WORK_NOTIFICATION: Schedule a Notify(device) firmware request for
 *                            execution. This can run on any CPU.
 */
uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx
) {
    (void) type;
    // For now, just execute the work synchronously
    if (handler) {
        handler(ctx);
    }
    return UACPI_STATUS_OK;
}

/*
 * Waits for two types of work to finish:
 * 1. All in-flight interrupts installed via uacpi_kernel_install_interrupt_handler
 * 2. All work scheduled via uacpi_kernel_schedule_work
 *
 * Note that the waits must be done in this order specifically.
 */
uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    // All work is synchronous for now
    return UACPI_STATUS_OK;
}

/*
 * ============================================================================
 * Firmware Request Handling
 * ============================================================================
 */

/*
 * Handle a firmware request.
 *
 * Currently either a Breakpoint or Fatal operators.
 */
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    (void) req;
    log_warning("Firmware request not implemented");
    return UACPI_STATUS_OK;
}

#endif
