#ifndef SHAPES_H
#define SHAPES_H

#include "canvas.h"

#define MAX_SHAPES 100

// Shape types
typedef enum {
    SHAPE_LINE,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;

// Core coordinate structure
typedef struct {
    int x;
    int y;
} Point;

// Shapes parameter unions
typedef struct {
    Point start;
    Point end;
} LineParams;

typedef struct {
    Point top_left;
    int width;
    int height;
} RectParams;

typedef struct {
    Point center;
    int radius;
} CircleParams;

typedef struct {
    Point p1;
    Point p2;
    Point p3;
} TriangleParams;

// Shape wrapper
typedef struct {
    int id;
    ShapeType type;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriangleParams triangle;
    } params;
    char draw_char;
    ShapeColor color;
    int active;
} Shape;

// Global shape collection
extern Shape shapes[MAX_SHAPES];
extern int next_shape_id;

// Shape API
void init_shapes(void);
int add_shape(ShapeType type, const void *params, char draw_char, ShapeColor color);
int delete_shape(int id);
Shape* find_shape(int id);
void render_all_shapes(void);
void list_active_shapes(void);

#endif // SHAPES_H
