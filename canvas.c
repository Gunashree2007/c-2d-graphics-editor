#include "canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Definitions of global canvas state variables
char canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
unsigned char canvas_color[CANVAS_HEIGHT][CANVAS_WIDTH];
char bg_char = '_';

// Initialize/clear the canvas
void init_canvas(char bg) {
    bg_char = bg;
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            canvas[y][x] = bg;
            canvas_color[y][x] = SHAPE_COLOR_DEFAULT;
        }
    }
}

// Plot a single pixel on the canvas with bounds checking
void draw_pixel(int x, int y, char ch, ShapeColor color) {
    if (x >= 0 && x < CANVAS_WIDTH && y >= 0 && y < CANVAS_HEIGHT) {
        canvas[y][x] = ch;
        canvas_color[y][x] = color;
    }
}

// Bresenham's Line Algorithm
void draw_line(int x0, int y0, int x1, int y1, char ch, ShapeColor color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        draw_pixel(x0, y0, ch, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Draw a rectangle boundary
void draw_rectangle(int x, int y, int w, int h, char ch, ShapeColor color) {
    if (w <= 0 || h <= 0) return;
    // Top and bottom horizontal segments
    for (int i = 0; i < w; i++) {
        draw_pixel(x + i, y, ch, color);
        draw_pixel(x + i, y + h - 1, ch, color);
    }
    // Left and right vertical segments
    for (int i = 0; i < h; i++) {
        draw_pixel(x, y + i, ch, color);
        draw_pixel(x + w - 1, y + i, ch, color);
    }
}

// Helper: 8-way symmetric plotting for circles
static void plot_circle_points(int xc, int yc, int x, int y, char ch, ShapeColor color) {
    draw_pixel(xc + x, yc + y, ch, color);
    draw_pixel(xc - x, yc + y, ch, color);
    draw_pixel(xc + x, yc - y, ch, color);
    draw_pixel(xc - x, yc - y, ch, color);
    draw_pixel(xc + y, yc + x, ch, color);
    draw_pixel(xc - y, yc + x, ch, color);
    draw_pixel(xc + y, yc - x, ch, color);
    draw_pixel(xc - y, yc - x, ch, color);
}

// Midpoint Circle Algorithm
void draw_circle(int xc, int yc, int r, char ch, ShapeColor color) {
    if (r < 0) return;
    if (r == 0) {
        draw_pixel(xc, yc, ch, color);
        return;
    }
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    plot_circle_points(xc, yc, x, y, ch, color);
    while (y >= x) {
        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
        plot_circle_points(xc, yc, x, y, ch, color);
    }
}

// Draw a triangle by connecting its three corners
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, char ch, ShapeColor color) {
    draw_line(x1, y1, x2, y2, ch, color);
    draw_line(x2, y2, x3, y3, ch, color);
    draw_line(x3, y3, x1, y1, ch, color);
}

// Draw the entire canvas to stdout with borders and ruler lines
void display_canvas(void) {
    // Print column ruler (tens digit)
    printf("   ");
    for (int x = 0; x < CANVAS_WIDTH; x++) {
        if (x % 10 == 0) {
            printf("%d", (x / 10) % 10);
        } else {
            printf(" ");
        }
    }
    printf("\n");

    // Print column ruler (units digit)
    printf("   ");
    for (int x = 0; x < CANVAS_WIDTH; x++) {
        printf("%d", x % 10);
    }
    printf("\n");

    // Print top border
    printf("  +");
    for (int x = 0; x < CANVAS_WIDTH; x++) {
        printf("-");
    }
    printf("+\n");

    // Print canvas rows
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        printf("%2d|", y);
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            ShapeColor col = canvas_color[y][x];
            switch (col) {
                case SHAPE_COLOR_RED:     printf("\033[1;31m"); break;
                case SHAPE_COLOR_GREEN:   printf("\033[1;32m"); break;
                case SHAPE_COLOR_YELLOW:  printf("\033[1;33m"); break;
                case SHAPE_COLOR_BLUE:    printf("\033[1;34m"); break;
                case SHAPE_COLOR_MAGENTA: printf("\033[1;35m"); break;
                case SHAPE_COLOR_CYAN:    printf("\033[1;36m"); break;
                case SHAPE_COLOR_WHITE:   printf("\033[1;37m"); break;
                default:            break;
            }
            printf("%c", canvas[y][x]);
            if (col != SHAPE_COLOR_DEFAULT) {
                printf("\033[0m"); // Reset back to default
            }
        }
        printf("|\n");
    }

    // Print bottom border
    printf("  +");
    for (int x = 0; x < CANVAS_WIDTH; x++) {
        printf("-");
    }
    printf("+\n");
}
