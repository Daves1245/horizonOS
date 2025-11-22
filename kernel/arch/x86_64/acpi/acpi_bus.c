#include <x86_64/acpi/acpi_bus.h>
#include <uacpi/types.h>
#include <kernel/logger.h>

static struct acpi_driver *acpi_drivers_head;

void acpi_register_driver(struct acpi_driver *driver) {
    struct acpi_driver *next = acpi_drivers_head;
    acpi_drivers_head = driver;
    driver->next = next;
}

static uacpi_iteration_decision acpi_init_one_device(void *ctx, uacpi_namespace_node *node,
        uacpi_u32 node_depth) {
    (void)ctx;
    (void)node_depth;

    uacpi_namespace_node_info *info;

    uacpi_status ret = uacpi_get_namespace_node_info(node, &info);
    if (uacpi_unlikely_error(ret)) {
        const char *path = uacpi_namespace_node_generate_absolute_path(node);
        log_error("unable to retrieve node information");
        uacpi_free_absolute_path(path);
        return UACPI_ITERATION_DECISION_CONTINUE;
    }

    struct acpi_driver *drv = NULL;

    if (info->flags & UACPI_NS_NODE_INFO_HAS_HID) {
        // match the HID against every existing acpi_driver pnp id list
    }

    if (drv == NULL && (info->flags & UACPI_NS_NODE_INFO_HAS_CID)) {
        // match the CID list against every existing acpi_driver pnp id list
    }

    if (drv != NULL) {
        // probe the driver and do something with the error code if desired
        drv->device_probe(node, info);
    }

    uacpi_free_namespace_node_info(info);
    return UACPI_ITERATION_DECISION_CONTINUE;
}

void acpi_bus_enumerate() {
    uacpi_namespace_for_each_child(
            uacpi_namespace_root(), acpi_init_one_device, UACPI_NULL,
                UACPI_OBJECT_DEVICE_BIT, UACPI_MAX_DEPTH_ANY, UACPI_NULL
            );
}
