#ifndef ACPI_KEYBOARD_H
#define ACPI_KEYBOARD_H

#include <x86_64/acpi/acpi_driver.h>

struct acpi_driver keyboard_driver {
  .device_name = "";
  .pnp_ids = {"", ""};
  .device_probe = keyboard_driver_device_probe;
  .next = NULL;
}

#endif
