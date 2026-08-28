#include <x86_64/acpi/acpi_driver.h>
#include <x86_64/acpi/acpi_bus.h>
#include <string.h>
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

    struct acpi_driver *driver = NULL;

    // check each driver's pnp list to see if we support
    // the device's hid value
    if (info->flags & UACPI_NS_NODE_INFO_HAS_HID) {
        for (struct acpi_driver *driver_it = acpi_drivers_head; driver_it; driver_it = driver_it->next) {
            for (const char *const *id = driver_it->pnp_ids; *id; id++) {
                if (strcmp(info->hid.value, *id) == 0) {
                    driver = driver_it;
                    break;
                }
            }
            if (driver) {
                break;
            }
        }
    }

    // otherwise if we have a cid list, check it against every acpi driver
    if (!driver && (info->flags & UACPI_NS_NODE_INFO_HAS_CID)) {
        // match the CID list against every existing acpi_driver pnp id list
        for (struct acpi_driver *driver_it = acpi_drivers_head; driver_it; driver_it = driver_it->next) {
            for (uint32_t i = 0; i < info->cid.num_ids; i++) {
                for (const char *const *id = driver_it->pnp_ids; *id; id++) {
                    if (strcmp(*id, info->cid.ids[i].value) == 0) {
                        driver = driver_it;
                        break;
                    }
                }
                if (driver) {
                    break;
                }
            }
            if (driver) {
                break;
            }
        }
    }

    if (driver) {
        // probe the driver and do something with the error code if desired
        driver->device_probe(node, info);
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
