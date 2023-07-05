#include <math.h>
#include <stdio.h>

#include <mes.h>
#include <gpu.h>
#include <input.h>
#include <timer.h>

#include "graphics.h"
#include "tension.h"

#define DEBUG_SCALE 10
#define MTGX(X) (uint8_t) ((X*DEBUG_SCALE+80))
#define MTGY(Y) (uint8_t) ((60-Y*DEBUG_SCALE))
#define ROTATE_COOLDOWN 50
#define MOVE_COOLDOWN 50
#define PLAYER_STEP 0.05

#define VIEWPORT_WIDTH 160
#define VIEW_DISTANCE 100

#define CEILING_COLOR 3
#define FLOOR_COLOR 2
#define WALL_COLOR_1 4
#define WALL_COLOR_2 5

//#define DEBUG

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

    // map
    line l1 = {&(point){-1, 1}, &(point){1, 1}, WALL_COLOR_1}; // random shape
    line l2 = {&(point){1, 1}, &(point){1, -1}, WALL_COLOR_2};
    line l3 = {&(point){-1, 1}, &(point){-1, 2}, WALL_COLOR_2};
    line l4 = {&(point){-1, 2}, &(point){1.5, 2}, WALL_COLOR_1};
    line l5 = {&(point){1.5, 2}, &(point){1.5, -1}, WALL_COLOR_2};
    line l6 = {&(point){1.5, -1}, &(point){1, -1}, WALL_COLOR_1};
    line l7 = {&(point){-5, 3}, &(point){5, 3}, WALL_COLOR_1}; // border
    line l8 = {&(point){5, 3}, &(point){5, -3}, WALL_COLOR_2};
    line l9 = {&(point){5, -3}, &(point){-5, -3}, WALL_COLOR_1};
    line l10 = {&(point){-5, -3}, &(point){-5, 3}, WALL_COLOR_2};
    polygon pg = {(line[]) {l1, l2, l3, l4, l5, l6, l7, l8, l9, l10}, 10};

    // init camera
    camera cam = {&(point){0, 0}, &(point){0, 0}, &(point){0, 0}, 1.0, 0.5, 0};
    camera_rotate(&cam, 0);
    surface surf = surf_create(160, 120);

    // game loop
    int rotate_cooldown = -1;
    int move_cooldown = -1;
    int angle;
    while (true) {

        // draw ceiling & floor
        surf_draw_filled_rectangle(&surf, 0, 0, 160, 60, CEILING_COLOR);
        surf_draw_filled_rectangle(&surf, 0, 60, 160, 60, FLOOR_COLOR);

        // quit game
        if (input_get_button(0, BUTTON_SELECT)) break;

        // rotate
        if (input_get_button(0, BUTTON_A)) {
            if (rotate_cooldown < 0) {
                angle++;
                if (angle == 360) angle = 0;
                camera_rotate(&cam, angle);
                rotate_cooldown = ROTATE_COOLDOWN;
            }
            rotate_cooldown--;
        }
        if (input_get_button(0, BUTTON_B)) {
            if (rotate_cooldown < 0) {
                angle--;
                if (angle == 0) angle = 360;
                camera_rotate(&cam, angle);
                rotate_cooldown = ROTATE_COOLDOWN;
            }
            rotate_cooldown--;
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
        } else move_cooldown--;

        // render
        float length = point_length(cam.left, cam.right)/VIEWPORT_WIDTH;
        float dx = (cam.right->x - cam.left->x)/VIEWPORT_WIDTH;
        float dy = (cam.right->y - cam.left->y)/VIEWPORT_WIDTH;
        for (int i = 0; i < VIEWPORT_WIDTH; i++) {
            point p = {cam.left->x + i*dx, cam.left->y + i*dy};

            // view distance
            float pdx = p.x - cam.position->x;
            float pdy = p.y - cam.position->y;
            p.x += pdx * VIEW_DISTANCE;
            p.y += pdy * VIEW_DISTANCE;

            point intersection = {0, 0};
            float lowscore = 300;
            for (int j = 0; j < pg.size; j++) {
                line current = pg.lines[j];
                if (point_intersection(cam.position, &p, current.p1, current.p2, &intersection)) {
                    float len = point_length(cam.position, &intersection);
                    if (len < lowscore) {
                        lowscore = len;
                        #ifdef DEBUG
                            uint8_t ix = MTGX(intersection.x);
                            uint8_t iy = MTGY(intersection.y);
                            uint8_t px = MTGX(cam.position->x);
                            uint8_t py = MTGY(cam.position->y);
                            surf_draw_line(&surf, px, py, ix, iy, 1);
                        #endif
                        #ifndef DEBUG
                            uint8_t lineheight = 60/len;
                            surf_draw_line(&surf, i, 60, i, 60-lineheight, current.color);
                            surf_draw_line(&surf, i, 60, i, 60+lineheight, current.color);
                        #endif
                    }
                }
            }
        }

        // debug
        #ifdef DEBUG
            uint8_t c1x = MTGX(cam.left->x);
            uint8_t c1y = MTGY(cam.left->y);
            uint8_t c2x = MTGX(cam.right->x);
            uint8_t c2y = MTGY(cam.right->y);
            uint8_t px = MTGX(cam.position->x);
            uint8_t py = MTGY(cam.position->y);
            surf_draw_line(&surf, c1x, c1y, c2x, c2y, 1);
            surf_set_pixel(&surf, c1x, c1y, 2);
            surf_set_pixel(&surf, c2x, c2y, 2);
            surf_set_pixel(&surf, px, py, 2);
            for (int i = 0; i < pg.size; i++) {
                line current = pg.lines[i];
                uint8_t p1x = MTGX(current.p1->x);
                uint8_t p1y = MTGY(current.p1->y);
                uint8_t p2x = MTGX(current.p2->x);
                uint8_t p2y = MTGY(current.p2->y);
                surf_draw_line(&surf, p1x, p1y, p2x, p2y, 1);
            }
        #endif

        // send to gpu
        gpu_send_buf(FRONT_BUFFER, surf.w, surf.h, 0, 0, surf.data);

        // no timing for now ... the game relies on lag to work.
    }

    return CODE_FREEZEFRAME;
}
