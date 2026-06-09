#ifndef CANVAS_H
#define CANVAS_H

#define CANVAS_WIDTH 60
#define CANVAS_HEIGHT 12

// Shape color enumeration mapping to ANSI Terminal Colors
typedef enum {
    SHAPE_COLOR_DEFAULT,
    SHAPE_COLOR_RED,
    SHAPE_COLOR_GREEN,
    SHAPE_COLOR_YELLOW,
    SHAPE_COLOR_BLUE,
    SHAPE_COLOR_MAGENTA,
    SHAPE_COLOR_CYAN,
    SHAPE_COLOR_WHITE
} ShapeColor;

// Global canvas arrays
extern char canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
extern unsigned char canvas_color[CANVAS_HEIGHT][CANVAS_WIDTH];
extern char bg_char;

// Canvas operations
void init_canvas(char bg);
void draw_pixel(int x, int y, char ch, ShapeColor color);

// Shape drawing algorithms
void draw_line(int x0, int y0, int x1, int y1, char ch, ShapeColor color);
void draw_rectangle(int x, int y, int w, int h, char ch, ShapeColor color);
void draw_circle(int xc, int yc, int r, char ch, ShapeColor color);
void draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, char ch, ShapeColor color);

// Display operation
void display_canvas(void);

#endif // CANVAS_H
