
#pragma once

#define MOUSE_LB (1 << 0)
#define MOUSE_RB (1 << 1)
#define MOUSE_MB (1 << 2)
#define MOUSE_B4 (1 << 3)
#define MOUSE_B5 (1 << 4)

typedef struct {
    char buttons;
    char x;
    char y;
    char z;
} mouse_ev_t;

void enable_mouse_test();
void handle_mouse_event(mouse_ev_t event);