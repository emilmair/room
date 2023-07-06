#include <math.h>
#include <stdio.h>

#include <mes.h>
#include <gpu.h>
#include <input.h>
#include <timer.h>

#include "maths.h"
#include "strings.h"
#include "graphics.h"
#include "tension.h"

#define ROTATE_COOLDOWN 0
#define MOVE_COOLDOWN 0
#define PLAYER_STEP 0.05

#define WALL_COLOR_1 4
#define WALL_COLOR_2 5

uint8_t start(void) {

    // palette
    uint16_t* grayscale = malloc(16);
    grayscale[0] = _COLOR(0b000, 0b000, 0b000);
    grayscale[1] = _COLOR(0b001, 0b001, 0b001);
    grayscale[2] = _COLOR(0b010, 0b010, 0b010);
    grayscale[3] = _COLOR(0b011, 0b011, 0b011);
    grayscale[4] = _COLOR(0b100, 0b100, 0b100);
    grayscale[5] = _COLOR(0b101, 0b101, 0b101);
    grayscale[6] = _COLOR(0b110, 0b110, 0b110);
    grayscale[7] = _COLOR(0b111, 0b111, 0b111);
    gpu_update_palette(grayscale);
    free(grayscale);

    // map
    point* points = malloc(sizeof(point)*10);
    line* lines = malloc(sizeof(line)*10);
    points[0] = (point){-1, 1}; // L-shape
    points[1] = (point){1, 1};
    points[2] = (point){1, -1};
    points[3] = (point){1.5, -1};
    points[4] = (point){1.5, 2};
    points[5] = (point){-1, 2};
    points[6] = (point){-5, 3}; // border
    points[7] = (point){5, 3};
    points[8] = (point){5, -3};
    points[9] = (point){-5, -3};
    lines[0] = (line){&points[0], &points[1], WALL_COLOR_1};
    lines[1] = (line){&points[1], &points[2], WALL_COLOR_2};
    lines[2] = (line){&points[2], &points[3], WALL_COLOR_1};
    lines[3] = (line){&points[3], &points[4], WALL_COLOR_2};
    lines[4] = (line){&points[4], &points[5], WALL_COLOR_1};
    lines[5] = (line){&points[5], &points[0], WALL_COLOR_2};
    lines[6] = (line){&points[6], &points[7], WALL_COLOR_1};
    lines[7] = (line){&points[7], &points[8], WALL_COLOR_2};
    lines[8] = (line){&points[8], &points[9], WALL_COLOR_1};
    lines[9] = (line){&points[9], &points[6], WALL_COLOR_2};
    map m = {lines, 10, 3, 2};

    // init camera
    camera cam = {&(point){0, 0}, &(point){0, 0}, &(point){0, 0}, 1.0, 0.5, 0};
    camera_rotate(&cam, 0);
    surface surf = surf_create(160, 120);

    // game loop
    int rotate_cooldown = -1;
    int move_cooldown = -1;
    int angle;
    uint32_t start = 0;
    uint32_t stop = 0;
    uint32_t deltatime = 0;
    bool debug = false;
    bool last_debug;
    while (true) {

        // quit game
        if (input_get_button(0, BUTTON_SELECT)) break;

        // toggle debug
        if (input_get_button(0, BUTTON_START) && !last_debug) debug = !debug;
        last_debug = input_get_button(0, BUTTON_START);

        // rotate
        if (input_get_button(0, BUTTON_A)) {
            if (rotate_cooldown < 0) {
                angle++;
                if (angle == 360) angle = 0;
                camera_rotate(&cam, angle);
                rotate_cooldown = ROTATE_COOLDOWN;
            }
            rotate_cooldown -= deltatime;
        }
        if (input_get_button(0, BUTTON_B)) {
            if (rotate_cooldown < 0) {
                angle--;
                if (angle == 0) angle = 360;
                camera_rotate(&cam, angle);
                rotate_cooldown = ROTATE_COOLDOWN;
            }
            rotate_cooldown -= deltatime;
        }

        // move
        if (move_cooldown < 0) {
            move_cooldown = MOVE_COOLDOWN;
            point next_pos = {0, 0};
            if (input_get_button(0, BUTTON_UP)) next_pos.x += PLAYER_STEP;
            if (input_get_button(0, BUTTON_DOWN)) next_pos.x -= PLAYER_STEP;
            if (input_get_button(0, BUTTON_LEFT)) next_pos.y += PLAYER_STEP;
            if (input_get_button(0, BUTTON_RIGHT)) next_pos.y -= PLAYER_STEP;
            if (next_pos.x != 0 || next_pos.y != 0) {
                next_pos.x += cam.position->x;
                next_pos.y += cam.position->y;
                point_rotate(cam.position, &next_pos, cam.angle);
                camera_move(&cam, &next_pos);
            }
        } else move_cooldown -= deltatime;

        // render
        camera_render(&cam, &surf, &m, debug);

        // timing
        gpu_block_frame();
        stop = timer_get_ms();
        deltatime = stop - start;
        start = timer_get_ms();

         // send to gpu
        gpu_send_buf(BACK_BUFFER, surf.w, surf.h, 0, 0, surf.data);
        gpu_print_text(BACK_BUFFER, 0, 0, 7, 0, INLINE_DECIMAL3(deltatime));

        // wait for everything to be sent
        gpu_block_ack();
        gpu_swap_buf();
    }
    free(points);
    free(lines);
    return CODE_EXIT;
}
