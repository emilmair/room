#ifndef TENSION_TENSION_H
#define TENSION_TENSION_H

#include <stdbool.h>
#include "graphics.h"

#define RAD_CONV_FACTOR 0.0174532925
#define VIEW_DISTANCE 100

#define DEBUG_SCALE 10
#define MTSX(X, S) (uint8_t) ((X*DEBUG_SCALE+(S->w/2)))
#define MTSY(Y, S) (uint8_t) (((S->h/2)-Y*DEBUG_SCALE))

// BASIC TYPES ================================================================

typedef struct {
    float x;
    float y;
} point;

typedef struct {
    point* p1;
    point* p2;
    uint8_t color;
} line;

// MATHS      =================================================================

bool point_intersection(point* p1, point* p2, point* p3, point* p4, point* intersection) {
    point s1, s2;
    s1.x = p2->x - p1->x;     s1.y = p2->y - p1->y;
    s2.x = p4->x - p3->x;     s2.y = p4->y - p3->y;

    float s, t;
    s = (-s1.y * (p1->x - p3->x) + s1.x * (p1->y - p3->y)) / (-s2.x * s1.y + s1.x * s2.y);
    t = ( s2.x * (p1->y - p3->y) - s2.y * (p1->x - p3->x)) / (-s2.x * s1.y + s1.x * s2.y);

    // collision
    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
        if (intersection != NULL) {
            intersection->x = p1->x + (t * s1.x);
            intersection->y = p1->y + (t * s1.y);
        }
        return true;
    }

    // no collision
    return false;
}

bool line_intersection(line* l1, line* l2, point* intersection) {
    return point_intersection(l1->p1, l1->p2, l2->p1, l2->p2, intersection);
}

void point_rotate(point* p1, point* p2, uint16_t a) {
    float radians = a * RAD_CONV_FACTOR;
    p2->x -= p1->x;
    p2->y -= p1->y;
    float c = cos(radians);
    float s = sin(radians);
    float nx = p2->x * c - p2->y * s;
    float ny = p2->x * s + p2->y * c;
    p2->x = nx + p1->x;
    p2->y = ny + p1->y;
}

void line_rotate(point* p1, line* l1, uint16_t angle) {
    point_rotate(p1, l1->p1, angle);
    point_rotate(p1, l1->p2, angle);
}

float point_length(point* p1, point* p2) {
    return hypot(p2->x - p1->x, p2->y - p1->y);
}

float line_length(line* l1) {
    return point_length(l1->p1, l1->p2);
}

// TYPES =====================================================================

typedef struct {
    line* lines;
    unsigned int size;
    uint8_t ceiling_color;
    uint8_t floor_color;
} map;

typedef union {
    struct {
        point* position;
        point* left;
        point* right;
        float width;
        float distance;
        uint16_t angle;
    };
    struct {
        point* p;
        point* l;
        point* r;
        float w;
        float d;
        uint16_t a;
    };
} camera;

// FUNCTIONS =================================================================

void camera_rotate(camera* c, uint16_t a) {
    // start in 0 degree position
    c->l->x = c->p->x + c->w;
    c->l->y = c->p->y + c->d;
    c->r->x = c->p->x + c->w;
    c->r->y = c->p->y - c->d;

    // now change the angle
    point_rotate(c->p, c->l, a);
    point_rotate(c->p, c->r, a);
    c->a = a;
}

void camera_move(camera* c, point* p1) {
    float pdx = p1->x - c->p->x;
    float pdy = p1->y - c->p->y;
    c->p->x = p1->x;
    c->p->y = p1->y;
    c->l->x += pdx;
    c->l->y += pdy;
    c->r->x += pdx;
    c->r->y += pdy;
}

void camera_render(camera* c, surface* s, map* m, bool debug) {
    surf_draw_filled_rectangle(s, 0, 0, 160, 60, m->ceiling_color);
    surf_draw_filled_rectangle(s, 0, 60, 160, 60, m->floor_color);
    float length = point_length(c->l, c->r)/s->w;
    float dx = (c->r->x - c->l->x)/s->w;
    float dy = (c->r->y - c->l->y)/s->w;
    for (int i = 0; i < s->w; i++) {
        point p = {c->l->x + i*dx, c->l->y + i*dy};

        // view distance
        float pdx = p.x - c->p->x;
        float pdy = p.y - c->p->y;
        p.x += pdx * VIEW_DISTANCE;
        p.y += pdy * VIEW_DISTANCE;

        point intersection = {0, 0};
        float lowscore = 300;
        for (int j = 0; j < m->size; j++) {
            line* current = &m->lines[j];
            if (point_intersection(c->p, &p, current->p1, current->p2, &intersection)) {
                float len = point_length(c->p, &intersection);
                if (len < lowscore) {
                    lowscore = len;
                    uint8_t lineheight = (uint8_t)(60/len);
                    surf_draw_line(s, i, 60, i, 60-lineheight, current->color);
                    surf_draw_line(s, i, 60, i, 60+lineheight, current->color);
                }
            }
        }
    }

    if (debug) {
        uint8_t c1x = MTSX(c->l->x, s);
        uint8_t c1y = MTSY(c->l->y, s);
        uint8_t c2x = MTSX(c->r->x, s);
        uint8_t c2y = MTSY(c->r->y, s);
        uint8_t px = MTSX(c->p->x, s);
        uint8_t py = MTSY(c->p->y, s);
        surf_draw_line(s, c1x, c1y, c2x, c2y, 6);
        surf_set_pixel(s, c1x, c1y, 7);
        surf_set_pixel(s, c2x, c2y, 7);
        surf_set_pixel(s, px, py, 7);
        for (int i = 0; i < m->size; i++) {
            line* current = &m->lines[i];
            uint8_t p1x = MTSX(current->p1->x, s);
            uint8_t p1y = MTSY(current->p1->y, s);
            uint8_t p2x = MTSX(current->p2->x, s);
            uint8_t p2y = MTSY(current->p2->y, s);
            surf_draw_line(s, p1x, p1y, p2x, p2y, 6);
        }
    }
}

#endif //TENSION_TENSION_H
