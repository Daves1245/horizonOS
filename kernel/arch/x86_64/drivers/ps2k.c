#include "ps2k.h"
#include <x86_64/acpi/acpi_bus.h>
#include <stddef.h>
#include <stdio.h>
#include <kernel/logger.h>
#include <kernel/types.h>
#include <kernel/panic.h>
#include <x86_64/interrupts/isr.h>
#include <uacpi/utilities.h>
#include <uacpi/resources.h>
#include <acpi/acpi_driver.h>
#include <drivers/io.h>
#include <drivers/serial.h>
#include <apic/apic.h>

#define PS2K_PNP_ID "PNP0303"

extern void *ioapic_addr;
extern uint8_t local_apic_id;

static const char *const ps2k_pnp_ids[] = {
    PS2K_PNP_ID,
    NULL,
};

static struct ps2k_resources_t {
    uint8_t irq;
    uint16_t port_data;
    uint16_t port_cmd;
} ps2k_resources;

static char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    1, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 2,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 3, '*', 4, ' '
};

static char scancode_to_ascii_uppercase[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    1, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 2,
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 3, '*', 4, ' '
};

int key_pressed[256] = {0};

int is_key_pressed(enum KeyCode code) {
    return key_pressed[code];
}

static int shift_pressed = 0;
static int e0_prefix = 0;

static void wait_for_kbd_input(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if ((inb(ps2k_resources.port_cmd) & 0x02) == 0)
            return;
    }
}

static int wait_for_kbd_output(void) {
    int timeout = 100000;
    while (timeout-- > 0) {
        if (inb(ps2k_resources.port_cmd) & 0x01)
            return 1;
    }
    return 0;
}

static void ps2k_irq_handler(struct interrupt_context *regs) {
    (void) regs;
    uint8_t scancode = inb(ps2k_resources.port_data);

    if (scancode == 0xE0) {
        e0_prefix = 1;
        apic_send_eoi();
        return;
    }

    uint8_t index = e0_prefix ? (scancode & 0x7F) | 0x80 : scancode & 0x7F;
    int released = scancode & 0x80;

    if (released) {
        key_pressed[index] = 0;
        if (index == 0x2A || index == 0x36)
            shift_pressed = 0;
    } else {
        key_pressed[index] = 1;
        if (index == 0x2A || index == 0x36) {
            shift_pressed = 1;
        } else if (index < sizeof(scancode_to_ascii)) {
            char c = shift_pressed
                ? scancode_to_ascii_uppercase[index]
                : scancode_to_ascii[index];
            if (c != 0) {
                serial_putchar(c);
            }
        }
    }

    e0_prefix = 0;
    apic_send_eoi();
}

static void init_ps2k_driver(void) {

#ifdef DEBUG
    log_debug("[keyboard]: initializing PS/2 controller...\n");
#endif

    // disable both PS/2 ports
    wait_for_kbd_input();
    outb(ps2k_resources.port_cmd, 0xAD);  // disable first port
    wait_for_kbd_input();
    outb(ps2k_resources.port_cmd, 0xA7);  // disable second port

    // flush output buffer
    inb(ps2k_resources.port_data);

    // read controller configuration byte
    wait_for_kbd_input();
    outb(ps2k_resources.port_cmd, 0x20);
    if (wait_for_kbd_output()) {
        uint8_t config = inb(ps2k_resources.port_data);

        // enable keyboard interrupt (bit 0) and translation (bit 6)
        config |= 0x01;   // enable keyboard interrupt
        config |= 0x40;   // enable translation to scan code set 1

        // write back configuration
        wait_for_kbd_input();
        outb(ps2k_resources.port_cmd, 0x60);
        wait_for_kbd_input();
        outb(ps2k_resources.port_data, config);
    }

    // perform controller self-test
    wait_for_kbd_input();
    outb(ps2k_resources.port_cmd, 0xAA);
    if (wait_for_kbd_output()) {
        uint8_t result = inb(ps2k_resources.port_data);
        if (result == 0x55) {
#ifdef DEBUG
            log_success("[keyboard]: controller self-test passed\n");
#endif
        } else {
            log_error("Controller self-test failed: ");
            printf("0x%x\n", result);
            panic("ps2k controller failure");
        }
    }

    // test keyboard port
    wait_for_kbd_input();
    outb(ps2k_resources.port_cmd, 0xAB);
    if (wait_for_kbd_output()) {
        uint8_t result = inb(ps2k_resources.port_data);
        if (result == 0x00) {
#ifdef DEBUG
            log_success("[keyboard]: keyboard port test passed\n");
#endif
        } else {
            log_error("Keyboard port test failed: ");
            printf("0x%x\n", result);
        }
    }

    // enable keyboard port
    wait_for_kbd_input();
    outb(ps2k_resources.port_cmd, 0xAE);

    // reset keyboard device
    wait_for_kbd_input();
    outb(ps2k_resources.port_data, 0xFF);
    if (wait_for_kbd_output()) {
        uint8_t ack = inb(ps2k_resources.port_data);
        if (ack == 0xFA) {
#ifdef DEBUG
            log_debug("[keyboard]: keyboard reset ACK received\n");
#endif
            // wait for self-test result
            if (wait_for_kbd_output()) {
                uint8_t result = inb(ps2k_resources.port_data);
                if (result == 0xAA) {
#ifdef DEBUG
                    log_success("[keyboard]: keyboard self-test passed\n");
#endif
                } else {
                    log_error("[keyboard]: keyboard self-test result: ");
                    printf("0x%x\n", result);
                    panic("ps2k controller failure");
                }
            }
        } else {
            log_error("unexpected response to reset: ");
            printf("0x%x\n", ack);
            panic("ps2k controller failure");
        }
    }

    // enable scanning
    wait_for_kbd_input();
    outb(ps2k_resources.port_data, 0xF4);
    if (wait_for_kbd_output()) {
        uint8_t ack = inb(ps2k_resources.port_data);
        if (ack == 0xFA) {
#ifdef DEBUG
            log_success("[keyboard]: scanning enabled\n");
#endif
        } else {
            log_error("[keyboard]: failed to enable scanning: ");
            printf("0x%x\n", ack);
            panic("ps2k controller failure");
        }
    }

    log_success("[keyboard]: PS/2 controller initialized\n");
}

static int ps2k_create_device(void) {
    log_info("[ps2k] IRQ=%d data=0x%x cmd=0x%x\n",
             ps2k_resources.irq, ps2k_resources.port_data, ps2k_resources.port_cmd);

    uint8_t vector = 32 + ps2k_resources.irq;
    log_info("[ps2k] routing GSI %d -> vector %d, APIC id %d\n",
             ps2k_resources.irq, vector, local_apic_id);
    register_interrupt_handler(vector, ps2k_irq_handler);

    init_ps2k_driver();

    log_info("[ps2k] configuring IOAPIC routing\n");
    configure_ioapic_irq(ps2k_resources.irq, vector, local_apic_id);

    log_success("[ps2k] device ready\n");
    return 0;
}

uacpi_iteration_decision ps2k_handle_resource(void *user, uacpi_resource *resource) {
    struct ps2k_resources_t *res = user;
    switch (resource->type) {
	// UACPI_RESOURCE_TYPE_IO means a *flexible* port range, ps2k keyboard
	// has a fixed port io
	// also, since uacpi *must* follow the exact order the resources
	// are laid out in AML, it is guaranteed the data port will be specified
	// before the cmd port
	case UACPI_RESOURCE_TYPE_FIXED_IO:
	    if (!res->port_data) {
		res->port_data = resource->fixed_io.address;
	    } else {
		res->port_cmd = resource->fixed_io.address;
	    }
	    break;
	case UACPI_RESOURCE_TYPE_IO:
	    if (!res->port_data) {
		res->port_data = resource->io.minimum;
	    } else {
		res->port_cmd = resource->io.minimum;
	    }
	    break;
	case UACPI_RESOURCE_TYPE_IRQ:
	    res->irq = resource->irq.irqs[0];
	    break;
	case UACPI_RESOURCE_TYPE_EXTENDED_IRQ:
	    res->irq = resource->extended_irq.irqs[0];
	    break;
	case UACPI_RESOURCE_TYPE_END_TAG:
	    return UACPI_ITERATION_DECISION_BREAK;
	default:
	    break;
    }
    return UACPI_ITERATION_DECISION_CONTINUE;
}

static int ps2k_probe(uacpi_namespace_node *node, uacpi_namespace_node_info *info) {
    (void) info;
    log_info("[ps2k] probing device (HID=%s)\n", info->hid.value);

    uacpi_resources *kb_res;
    uacpi_status st = uacpi_get_current_resources(node, &kb_res);
    if (uacpi_unlikely_error(st)) {
	log_error("[ps2k] unable to retrieve resources\n");
	return -ENODEV;
    }

    uacpi_for_each_resource(kb_res, ps2k_handle_resource, (void *) &ps2k_resources);

    int ret = ps2k_create_device();
    uacpi_free_resources(kb_res);
    return ret;
}

static struct acpi_driver ps2k_driver = {
    .device_name = "ps2 Keyboard",
    .pnp_ids = ps2k_pnp_ids,
    .device_probe = ps2k_probe
};

void ps2k_register(void) {
    acpi_register_driver(&ps2k_driver);
}
