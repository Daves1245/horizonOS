#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#include <uacpi/event.h>
#include <kernel/logger.h>
#include <kernel/types.h>

#ifdef UACPI_BAREBONES_MODE

/*
 * Barebones mode initialization - provides only table access, no AML interpreter.
 * This is suitable for early boot or when you only need to parse ACPI tables.
 */
int acpi_init(void) {
    uacpi_status ret;

    // Static buffer for early table access (4KB should handle most systems)
    static char table_buffer[4096];

    log_info("[uACPI] Initializing in barebones mode (tables-only)...\n");

    /*
     * Initialize early table access. This allows us to find and parse ACPI tables
     * without requiring a full heap allocator or other complex subsystems.
     */
    ret = uacpi_setup_early_table_access(table_buffer, sizeof(table_buffer));
    if (uacpi_unlikely_error(ret)) {
        log_error("[uACPI] Failed to setup early table access: %s\n",
                  uacpi_status_to_string(ret));
        return -ENODEV;
    }

    log_info("[uACPI] Barebones mode initialized successfully!\n");
    log_info("[uACPI] Table parsing API is now available.\n");
    log_info("[uACPI] Note: AML interpreter is NOT available in barebones mode.\n");

    return 0;
}

#else

/*
 * Full mode initialization - provides complete ACPI support with AML interpreter.
 * This requires all kernel API functions to be implemented.
 * (Ripped from https://wiki.osdev.org/UACPI)
 */
int acpi_init(void) {
    /*
     * Start with this as the first step of the initialization. This loads all
     * tables, brings the event subsystem online, and enters ACPI mode. We pass
     * in 0 as the flags as we don't want to override any default behavior for now.
     */
    uacpi_status ret = uacpi_initialize(0);
    if (uacpi_unlikely_error(ret)) {
        log_error("uacpi_initialize error: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    /*
     * Load the AML namespace. This feeds DSDT and all SSDTs to the interpreter
     * for execution.
     */
    ret = uacpi_namespace_load();
    if (uacpi_unlikely_error(ret)) {
        log_error("uacpi_namespace_load error: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    /*
     * Initialize the namespace. This calls all necessary _STA/_INI AML methods,
     * as well as _REG for registered operation region handlers.
     */
    ret = uacpi_namespace_initialize();
    if (uacpi_unlikely_error(ret)) {
        log_error("uacpi_namespace_initialize error: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    /*
     * Tell uACPI that we have marked all GPEs we wanted for wake (even though we haven't
     * actually marked any, as we have no power management support right now). This is
     * needed to let uACPI enable all unmarked GPEs that have a corresponding AML handler.
     * These handlers are used by the firmware to dynamically execute AML code at runtime
     * to e.g. react to thermal events or device hotplug.
     */
    ret = uacpi_finalize_gpe_initialization();
    if (uacpi_unlikely_error(ret)) {
        log_error("uACPI GPE initialization error: %s", uacpi_status_to_string(ret));
        return -ENODEV;
    }

    /*
     * That's it, uACPI is now fully initialized and working! You can proceed to
     * using any public API at your discretion. The next recommended step is namespace
     * enumeration and device discovery so you can bind drivers to ACPI objects.
     */
    return 0;
}

#endif
