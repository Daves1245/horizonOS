#ifndef ACPI_BUS_H
#define ACPI_BUS_H

#include <uacpi/uacpi.h>
#include <uacpi/namespace.h>
#include <uacpi/utilities.h>
#include <uacpi/resources.h>

struct acpi_driver {
  const char *device_name;
  const char *const *pnp_ids;
  int (*device_probe)(uacpi_namespace_node *node, uacpi_namespace_node_info *info);

  struct acpi_driver *next;
};

void acpi_register_driver(struct acpi_driver *driver);

#endif
