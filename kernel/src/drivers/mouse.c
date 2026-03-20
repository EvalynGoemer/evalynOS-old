#include <drivers/mouse.h>
#include <stdio.h>

#include <drivers/fb_renderer.h>

int is_mouse_test = 0;
int cursor_x = 0;
int cursor_y = 0;

extern int FB_WIDTH;
extern int FB_HEIGHT;

void draw_cursor() {
    int x = cursor_x;
    int y =  cursor_y;
    unsigned int color = 0xFFFFFF; 

    for (int i = -5; i <= 5; i++) {
        if (i != 0) { 
            plotPixel(x, y + i, color);
        }
    }

    for (int i = -5; i <= 5; i++) {
        if (i != 0) { 
            plotPixel(x + i, y, color);
        }
    }

    plotPixel(x, y, 0xFF0000);
}

void clear_cursor_area() {
    int size = 10;
    for (int i = -size; i <= size; i++) {
        for (int j = -size; j <= size; j++) {
            plotPixel(cursor_x + i,  cursor_y + j, 0x000000);
        }
    }
}

void update_cursor(int dx, int dy) {

    clear_cursor_area();

     cursor_x += dx;
     cursor_y += dy;

    if ( cursor_x < 0)  cursor_x = 0;
    if ( cursor_x >=  FB_WIDTH)  cursor_x =  FB_WIDTH - 1;
    if ( cursor_y < 0)  cursor_y = 0;
    if ( cursor_y >=  FB_HEIGHT)  cursor_y =  FB_HEIGHT - 1;

    draw_cursor();
}

void enable_mouse_test() {
    is_mouse_test = 1;
    update_cursor(0,0);
}

void handle_mouse_event(mouse_ev_t event) {
    // handle mouse logic here
    if(is_mouse_test) {
        update_cursor(event.x,event.y);
    }
}