#define TARGET_FPS 10
#define DELAY_MS (1000.0 / TARGET_FPS)
#define NUM_DEVICES 2

#define BOARD_HEIGHT (8 * NUM_DEVICES)
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>
#define LC_IMPLEMENTATION
#include "lc.h"

// PONG GAME

#define GAME_HEIGHT BOARD_HEIGHT
#define GAME_WIDTH 8
#define PADDLE_WIDTH 3
#define PADDLE_HEIGHT 1
#define BALL_SIZE 1
#define FILLED_ROWS 5

typedef struct world {
        int8_t x;
        int8_t y;
        int8_t dx;
        int8_t dy;
        int8_t paddle_x;

        uint8_t cells[FILLED_ROWS];
} world;

void world_init(world *w) {
    w->x = GAME_WIDTH / 2 - BALL_SIZE / 2;
    w->y = GAME_HEIGHT - PADDLE_HEIGHT - BALL_SIZE;
    w->dx = 0;
    w->dy = -1;
    w->paddle_x = GAME_WIDTH / 2 - PADDLE_WIDTH / 2;

    for (uint8_t i = 0; i < FILLED_ROWS; i++) {
        w->cells[i] = 0b11111111;
    }
}

int update(int8_t direction, world *w) {
    int8_t new_paddle_x = w->paddle_x + direction;
    if (new_paddle_x >= -1 && new_paddle_x <= GAME_WIDTH - PADDLE_WIDTH / 2) {
        w->paddle_x = new_paddle_x;
    }

    int8_t new_x = w->x + w->dx;
    int8_t new_y = w->y + w->dy;

    if (new_y >= GAME_HEIGHT) {
        w->dx = 0;
        w->dy = 0;
        w->y = new_y;
        return 1;
    }

    if (new_y < 0) {
        w->dx = rand() % 3 - 1;
        w->dy = -w->dy;
    } else if (new_y == GAME_HEIGHT - PADDLE_HEIGHT && w->x >= w->paddle_x &&
               w->x < w->paddle_x + PADDLE_WIDTH) {
        if (w->paddle_x <= w->x && w->x < w->paddle_x + PADDLE_WIDTH / 2) {
            w->dx = -1;
            w->dy = -w->dy;
        } else if (w->paddle_x + PADDLE_WIDTH / 2 < w->x &&
                   w->x < w->paddle_x + PADDLE_WIDTH) {
            w->dx = 1;
            w->dy = -w->dy;
        } else {
            w->dx = 0;
            w->dy = -w->dy;
        }
    } else {
        int collision = 0;
        if (new_y < FILLED_ROWS) {
            collision = w->cells[new_y] & (1 << new_x);
            if (collision) {
                w->cells[new_y] &= ~(1 << new_x);
                w->dx = rand() % 3 - 1;
                w->dy = -w->dy;
            }
        }

        if (new_x < 0 || new_x >= GAME_WIDTH) {
            w->dx = -w->dx;
        } else if (!collision) {
            w->x = new_x;
            w->y = new_y;
        }
    }

    return 0;
}

// END PONG GAME

#define RBUTTON_PIN PB0
#define LBUTTON_PIN PB1
#define DIN_PIN PB2
#define CS_PIN PB3
#define CLK_PIN PB4

uint8_t buffer[2 * NUM_DEVICES];

volatile int input = 0;
struct world w = {0};
struct led_controller controller;

ISR(INT0_vect) { input = -1; }
ISR(INT1_vect) { input = 1; }

void draw(const world *w, struct led_controller *controller) {
    uint8_t status[BOARD_HEIGHT] = {0};

    int8_t x = w->paddle_x;
    int8_t y = 0;

    for (uint8_t xi = 0; xi < PADDLE_WIDTH; xi++) {
        status[0] |= 1 << (x + xi);
    }

    for (uint8_t i = 0; i < FILLED_ROWS; i++) {
        status[BOARD_HEIGHT - i - 1] = w->cells[i];
    }

    status[BOARD_HEIGHT - w->y - 1] |= 1 << w->x;

    for (uint8_t i = 0; i < BOARD_HEIGHT; i++) {
        lc_set_row(controller, i / 8, i % 8, status[i]);
    }
}

int main() {
    int is_game_over = 0;

    // Led matrix setup
    lc_init(&controller, &DDRB, &PORTB, DIN_PIN, CS_PIN, CLK_PIN, NUM_DEVICES,
            buffer);

    for (uint8_t i = 0; i < NUM_DEVICES; i++) {
        lc_shutdown(&controller, i, 0);
        lc_set_intensity(&controller, i, 4);
        lc_clear_display(&controller, i);
    }

    // Tetris setup
    world_init(&w);

    // Input setup
    EICRA |= _BV(ISC00) | _BV(ISC01) | _BV(ISC10) | _BV(ISC11);
    EIMSK |= _BV(INT0) | _BV(INT1);

    sei();

    while (1) {
        if (update(input, &w) != 0) {
            world_init(&w);
            lc_clear_display(&controller, 0);
        }
        input = 0;

        draw(&w, &controller);

        _delay_ms(DELAY_MS);
    }

    return 0;
}
