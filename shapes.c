#include "shapes.h"
#include <stdio.h>
#include <string.h>

// Global shape state variables
Shape shapes[MAX_SHAPES];
int next_shape_id = 1;

// Initialize shape list
void init_shapes(void) {
    next_shape_id = 1;
    for (int i = 0; i < MAX_SHAPES; i++) {
        shapes[i].active = 0;
    }
}

// Add a shape to the collection
int add_shape(ShapeType type, const void *params, char draw_char, ShapeColor color) {
    int index = -1;
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (!shapes[i].active) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        printf("Error: Maximum shape limit (%d) reached!\n", MAX_SHAPES);
        return -1;
    }

    shapes[index].id = next_shape_id++;
    shapes[index].type = type;
    shapes[index].draw_char = draw_char;
    shapes[index].color = color;
    shapes[index].active = 1;

    switch (type) {
        case SHAPE_LINE:
            shapes[index].params.line = *(const LineParams*)params;
            break;
        case SHAPE_RECTANGLE:
            shapes[index].params.rect = *(const RectParams*)params;
            break;
        case SHAPE_CIRCLE:
            shapes[index].params.circle = *(const CircleParams*)params;
            break;
        case SHAPE_TRIANGLE:
            shapes[index].params.triangle = *(const TriangleParams*)params;
            break;
    }

    return shapes[index].id;
}

// Delete shape by ID
int delete_shape(int id) {
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active && shapes[i].id == id) {
            shapes[i].active = 0;
            return 1; // Deleted successfully
        }
    }
    return 0; // Not found
}

// Locate shape by ID
Shape* find_shape(int id) {
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active && shapes[i].id == id) {
            return &shapes[i];
        }
    }
    return NULL;
}

// Re-render all shapes on the canvas in order
void render_all_shapes(void) {
    init_canvas(bg_char);
    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active) {
            char ch = shapes[i].draw_char;
            ShapeColor color = shapes[i].color;
            switch (shapes[i].type) {
                case SHAPE_LINE: {
                    LineParams lp = shapes[i].params.line;
                    draw_line(lp.start.x, lp.start.y, lp.end.x, lp.end.y, ch, color);
                    break;
                }
                case SHAPE_RECTANGLE: {
                    RectParams rp = shapes[i].params.rect;
                    draw_rectangle(rp.top_left.x, rp.top_left.y, rp.width, rp.height, ch, color);
                    break;
                }
                case SHAPE_CIRCLE: {
                    CircleParams cp = shapes[i].params.circle;
                    draw_circle(cp.center.x, cp.center.y, cp.radius, ch, color);
                    break;
                }
                case SHAPE_TRIANGLE: {
                    TriangleParams tp = shapes[i].params.triangle;
                    draw_triangle(tp.p1.x, tp.p1.y, tp.p2.x, tp.p2.y, tp.p3.x, tp.p3.y, ch, color);
                    break;
                }
            }
        }
    }
}

// Format and list active shapes
void list_active_shapes(void) {
    printf("\n--- Active Shapes list ---\n");
    int count = 0;
    
    // Arrays for mapping colors to text names and terminal ANSI codes
    const char* col_names[] = {"Default", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"};
    const char* col_escapes[] = {"", "\033[1;31m", "\033[1;32m", "\033[1;33m", "\033[1;34m", "\033[1;35m", "\033[1;36m", "\033[1;37m"};

    for (int i = 0; i < MAX_SHAPES; i++) {
        if (shapes[i].active) {
            count++;
            printf("ID: %-3d | ", shapes[i].id);
            ShapeColor color = shapes[i].color;
            // Bounds check for color representation arrays
            int col_idx = (int)color;
            if (col_idx < 0 || col_idx >= 8) col_idx = 0;

            switch (shapes[i].type) {
                case SHAPE_LINE: {
                    LineParams lp = shapes[i].params.line;
                    printf("Line: (%d, %d) to (%d, %d) | Draw Char: '%c' | Color: %s%s\033[0m\n",
                           lp.start.x, lp.start.y, lp.end.x, lp.end.y,
                           shapes[i].draw_char, col_escapes[col_idx], col_names[col_idx]);
                    break;
                }
                case SHAPE_RECTANGLE: {
                    RectParams rp = shapes[i].params.rect;
                    printf("Rectangle: top-left(%d, %d), w:%d, h:%d | Draw Char: '%c' | Color: %s%s\033[0m\n",
                           rp.top_left.x, rp.top_left.y, rp.width, rp.height,
                           shapes[i].draw_char, col_escapes[col_idx], col_names[col_idx]);
                    break;
                }
                case SHAPE_CIRCLE: {
                    CircleParams cp = shapes[i].params.circle;
                    printf("Circle: center(%d, %d), r:%d | Draw Char: '%c' | Color: %s%s\033[0m\n",
                           cp.center.x, cp.center.y, cp.radius,
                           shapes[i].draw_char, col_escapes[col_idx], col_names[col_idx]);
                    break;
                }
                case SHAPE_TRIANGLE: {
                    TriangleParams tp = shapes[i].params.triangle;
                    printf("Triangle: p1(%d, %d), p2(%d, %d), p3(%d, %d) | Draw Char: '%c' | Color: %s%s\033[0m\n",
                           tp.p1.x, tp.p1.y, tp.p2.x, tp.p2.y, tp.p3.x, tp.p3.y,
                           shapes[i].draw_char, col_escapes[col_idx], col_names[col_idx]);
                    break;
                }
            }
        }
    }

    if (count == 0) {
        printf("(No shapes added yet)\n");
    }
    printf("---------------------------\n");
}
