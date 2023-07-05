#ifndef TENSION_TENSION_H
#define TENSION_TENSION_H

#include <stdbool.h>

#define PI 3.14159265
#define RAD_CONV_FACTOR 0.0174532925

// BASIC TYPES ================================================================

typedef struct _point {
    float x;
    float y;
} point;

typedef struct _line {
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

void point_rotate(point* center, point* p1, uint16_t angle) {
    float radians = angle * RAD_CONV_FACTOR;
    p1->x -= center->x;
    p1->y -= center->y;
    float c = cos(radians);
    float s = sin(radians);
    float nx = p1->x * c - p1->y * s;
    float ny = p1->x * s + p1->y * c;
    p1->x = nx + center->x;
    p1->y = ny + center->y;
}

void line_rotate(point* center, line* l1, uint16_t angle) {
    point_rotate(center, l1->p1, angle);
    point_rotate(center, l1->p2, angle);
}

float point_length(point* p1, point* p2) {
    return hypot(p2->x - p1->x, p2->y - p1->y);
}

float line_length(line* l1) {
    return point_length(l1->p1, l1->p2);
}

// TYPES =====================================================================

typedef struct _polygon {
    line* lines;
    unsigned int size;
} polygon;

typedef struct _map {
    polygon* polygons;
    unsigned int size;
} map;

typedef struct _camera {
    point* position;
    point* left;
    point* right;
    float width;
    float distance;
    uint16_t angle;
} camera;

// FUNCTIONS =================================================================

void camera_rotate(camera* c, uint16_t angle) {
    // start in 0 degree position
    c->left->x = c->position->x + c->width;
    c->left->y = c->position->y + c->distance;
    c->right->x = c->position->x + c->width;
    c->right->y = c->position->y - c->distance;

    // now change the angle
    point_rotate(c->position, c->left, angle);
    point_rotate(c->position, c->right, angle);
    c->angle = angle;
}

void camera_move(camera* c, point* p) {
    float pdx = p->x - c->position->x;
    float pdy = p->y - c->position->y;
    c->position->x = p->x;
    c->position->y = p->y;
    c->left->x += pdx;
    c->left->y += pdy;
    c->right->x += pdx;
    c->right->y += pdy;
}

#endif //TENSION_TENSION_H
