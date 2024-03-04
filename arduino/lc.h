#ifndef LC_H
#define LC_H

#include <avr/io.h>
#include <stdint.h>

#ifndef LCHDEF
#define LCHDEF
#endif

// the opcodes for the MAX7221 and MAX7219
#define OP_NOOP 0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE 9
#define OP_INTENSITY 10
#define OP_SCANLIMIT 11
#define OP_SHUTDOWN 12
#define OP_DISPLAYTEST 15

struct led_controller {
        volatile uint8_t *ddr;
        volatile uint8_t *port;
        uint8_t din_pin;
        uint8_t cs_pin;
        uint8_t clk_pin;
        uint8_t num_devices;
        uint8_t *buffer;
};

LCHDEF void lc_init(struct led_controller *controller, volatile uint8_t *ddr,
                    volatile uint8_t *port, uint8_t din_pin, uint8_t cs_pin,
                    uint8_t clk_pin, uint8_t num_devices, uint8_t *buffer);
LCHDEF void lc_set_scan_limit(struct led_controller *controller, uint8_t addr,
                              uint8_t limit);
LCHDEF void lc_set_intensity(struct led_controller *controller, uint8_t addr,
                             uint8_t intensity);
LCHDEF void lc_clear_display(struct led_controller *controller, uint8_t addr);
LCHDEF void lc_shutdown(struct led_controller *controller, uint8_t addr,
                        uint8_t b);
LCHDEF void lc_set_row(struct led_controller *controller, uint8_t addr,
                       uint8_t row, uint8_t value);

#endif // LC_H

#ifdef LC_IMPLEMENTATION

static void shift_out_msbfirst(volatile uint8_t *port, uint8_t din_pin,
                               uint8_t clk_pin, uint8_t data) {
    for (int i = 7; i >= 0; i--) {
        uint8_t mask = 1 << i;
        *port &= ~_BV(clk_pin);

        if (data & mask) {
            *port |= _BV(din_pin);
        } else {
            *port &= ~_BV(din_pin);
        }

        *port |= _BV(clk_pin);
    }
}

static void send_command(struct led_controller *controller, uint8_t addr,
                         volatile uint8_t opcode, volatile uint8_t data) {
    int offset = addr * 2;
    int maxbytes = controller->num_devices * 2;

    for (int i = 0; i < maxbytes; i++) {
        controller->buffer[i] = 0;
    }

    controller->buffer[offset + 1] = opcode;
    controller->buffer[offset] = data;

    *controller->port &= ~_BV(controller->cs_pin);

    for (int i = maxbytes - 1; i >= 0; i--) {
        shift_out_msbfirst(controller->port, controller->din_pin,
                           controller->clk_pin, controller->buffer[i]);
    }

    *controller->port |= _BV(controller->cs_pin);
}

// initialize the MAX7221 and MAX7219
//
// Arguments:
// controller: a pointer to the led_controller struct
// ddr: a pointer to the DDRx register
// port: a pointer to the PORTx register
// din_pin: the pin number for the data input
// cs_pin: the pin number for the chip select
// clk_pin: the pin number for the clock
// num_devices: the number of devices in the chain
// buffer: a pointer to a buffer to store the data to be sent to the devices
//         (must be at least 2 * num_devices bytes long)
LCHDEF void lc_init(struct led_controller *controller, volatile uint8_t *ddr,
                    volatile uint8_t *port, uint8_t din_pin, uint8_t cs_pin,
                    uint8_t clk_pin, uint8_t num_devices, uint8_t *buffer) {
    controller->ddr = ddr;
    controller->port = port;
    controller->din_pin = din_pin;
    controller->cs_pin = cs_pin;
    controller->clk_pin = clk_pin;
    controller->num_devices = num_devices;
    controller->buffer = buffer;

    *controller->ddr |= _BV(controller->din_pin) | _BV(controller->cs_pin) |
                        _BV(controller->clk_pin);

    *controller->port |= _BV(controller->cs_pin);

    for (int i = 0; i < num_devices; i++) {
        send_command(controller, i, OP_DISPLAYTEST, 0);
        lc_set_scan_limit(controller, i, 7);
        send_command(controller, i, OP_DECODEMODE, 0);
        lc_clear_display(controller, i);
        lc_shutdown(controller, i, 1);
    }
}

LCHDEF void lc_set_scan_limit(struct led_controller *controller, uint8_t addr,
                              uint8_t limit) {
    if (addr < 0 || addr >= controller->num_devices) {
        return;
    }
    if (limit >= 0 && limit < 8) {
        send_command(controller, addr, OP_SCANLIMIT, limit);
    }
}

// set the intensity of the display
//
// Arguments:
// controller: a pointer to the led_controller struct
// addr: the index of the device in the chain
// intensity: the intensity of the display (0-15)
//           0: minimum intensity
//           15: maximum intensity
LCHDEF void lc_set_intensity(struct led_controller *controller, uint8_t addr,
                             uint8_t intensity) {
    if (addr < 0 || addr >= controller->num_devices) {
        return;
    }
    if (intensity >= 0 && intensity < 16) {
        send_command(controller, addr, OP_INTENSITY, intensity);
    }
}

// clear the display
//
// Arguments:
// controller: a pointer to the led_controller struct
// addr: the index of the device in the chain
LCHDEF void lc_clear_display(struct led_controller *controller, uint8_t addr) {
    if (addr < 0 || addr >= controller->num_devices) {
        return;
    }

    for (uint8_t i = 0; i < 8; i++) {
        send_command(controller, addr, i + 1, 0);
    }
}

// turn the display on or off
//
// Arguments:
// controller: a pointer to the led_controller struct
// addr: the index of the device in the chain
// b: 1 to turn the display off, 0 to turn it on
LCHDEF void lc_shutdown(struct led_controller *controller, uint8_t addr,
                        uint8_t b) {
    if (addr < 0 || addr >= controller->num_devices) {
        return;
    }

    uint8_t value = b ? 0 : 1;

    send_command(controller, addr, OP_SHUTDOWN, value);
}

// set the value of a row
//
// Arguments:
// controller: a pointer to the led_controller struct
// addr: the index of the device in the chain
// row: the row to set (0-7)
// value: the value to set the row to
LCHDEF void lc_set_row(struct led_controller *controller, uint8_t addr,
                       uint8_t row, uint8_t value) {
    if (addr < 0 || addr >= controller->num_devices) {
        return;
    }
    if (row < 0 || row > 7) {
        return;
    }
    send_command(controller, addr, row + 1, value);
}

#endif // LC_IMPLEMENTATION
