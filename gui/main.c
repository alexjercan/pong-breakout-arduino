#include "raylib.h"

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define GAME_HEIGHT 16
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

#define CELL_SIZE 50
#define HEIGHT (GAME_HEIGHT * CELL_SIZE)
#define WIDTH (GAME_WIDTH * CELL_SIZE)

world w = {0};

void draw() {
    ClearBackground(BLACK);

    int paddle_position = w.paddle_x * CELL_SIZE;
    DrawRectangle(paddle_position, HEIGHT - PADDLE_HEIGHT * CELL_SIZE,
                  PADDLE_WIDTH * CELL_SIZE, PADDLE_HEIGHT * CELL_SIZE, RED);

    DrawRectangle(w.x * CELL_SIZE, w.y * CELL_SIZE, BALL_SIZE * CELL_SIZE,
                  BALL_SIZE * CELL_SIZE, WHITE);

    for (int i = 0; i < FILLED_ROWS; i++) {
        for (int j = 0; j < GAME_WIDTH; j++) {
            if (w.cells[i] & (1 << j)) {
                DrawRectangle(j * CELL_SIZE, i * CELL_SIZE, CELL_SIZE,
                              CELL_SIZE, GREEN);
            }
        }
    }
}

int main(void) {
    srand(time(NULL));
    world_init(&w);

    SetTargetFPS(15);

    InitWindow(WIDTH, HEIGHT, "Pong Game");

    while (!WindowShouldClose()) {
        int direction = 0;
        if (IsKeyPressed(KEY_LEFT))
            direction = direction - 1;
        if (IsKeyPressed(KEY_RIGHT))
            direction = direction + 1;

        if (update(direction, &w) != 0) {
            world_init(&w);
        }

        BeginDrawing();
        draw();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
