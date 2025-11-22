#include <x86_64/acpi/acpi_bus.h>
#include <kernel/logger.h>
#include <kernel/types.h>

#define PS2K_PNP_ID "PNP0303"

static const char *const ps2k_pnp_ids[] = {
    PS2K_PNP_ID,
    NULL,
};

static int ps2k_create_device(void) {
    // TODO: Implement actual PS/2 keyboard device creation
    log_info("PS/2 keyboard device detected (not yet implemented)");
    return 0;
}

static int ps2k_probe(uacpi_namespace_node *node, uacpi_namespace_node_info *info) {
    uacpi_resources *kb_res;

    /* Parse the resources to find the IRQ and IO ports the keyboard is connected to
     *
     * Note that for a centralized system like that the resources could be passed
     * to the device probe callback from common enumeration code at this point as
     * well!
     */
    uacpi_status st = uacpi_get_current_resources(node, &kb_res);
    if (uacpi_unlikely_error(st)) {
	log_error("unable to retrieve ps2k resources");
	return -ENODEV;
    }

    // instantiate the device
    int ret = ps2k_create_device();
    uacpi_free_resources(kb_res);
    return ret;
}

static struct acpi_driver ps2k_driver = {
    .device_name = "ps2 Keyboard",
    .pnp_ids = ps2k_pnp_ids,
    .device_probe = ps2k_probe
};
