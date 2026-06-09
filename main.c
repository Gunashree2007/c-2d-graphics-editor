#define NCURSES_STATIC
#include <ncurses/ncurses.h>
#include "canvas.h"
#include "shapes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sub-windows layout variables
WINDOW *canvas_win = NULL;
WINDOW *menu_win = NULL;

// Clamp coordinates within the valid canvas boundaries
void clamp_point(int *x, int *y) {
    if (*x < 0) *x = 0;
    if (*x >= CANVAS_WIDTH) *x = CANVAS_WIDTH - 1;
    if (*y < 0) *y = 0;
    if (*y >= CANVAS_HEIGHT) *y = CANVAS_HEIGHT - 1;
}

// Helper: safe numeric list reader inside ncurses
int read_ints_ncurses(WINDOW *win, int y, int x, const char *prompt, int *vals, int count) {
    wmove(win, y, x);
    wclrtoeol(win);
    wprintw(win, "%s", prompt);
    echo();
    curs_set(1);
    
    char buf[128];
    wgetnstr(win, buf, sizeof(buf) - 1);
    
    noecho();
    curs_set(0);
    
    int parsed = 0;
    char *token = strtok(buf, " \t,");
    while (token != NULL && parsed < count) {
        vals[parsed++] = atoi(token);
        token = strtok(NULL, " \t,");
    }
    return (parsed == count);
}

// Helper: safe single character reader inside ncurses
int read_char_ncurses(WINDOW *win, int y, int x, const char *prompt, char *value) {
    wmove(win, y, x);
    wclrtoeol(win);
    wprintw(win, "%s", prompt);
    echo();
    curs_set(1);
    
    char buf[128];
    wgetnstr(win, buf, sizeof(buf) - 1);
    
    noecho();
    curs_set(0);
    
    int i = 0;
    while (buf[i] == ' ' || buf[i] == '\t') i++;
    if (buf[i] == '\n' || buf[i] == '\0') {
        return 0;
    }
    *value = buf[i];
    return 1;
}

// Initialize ncurses color pairs mapping
void init_ncurses_colors(void) {
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(6, COLOR_CYAN, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
    }
}

// Render canvas cells into the canvas sub-window (flicker-free)
void display_canvas_ncurses(void) {
    werase(canvas_win);
    box(canvas_win, 0, 0);
    mvwprintw(canvas_win, 0, 2, " Drawing Canvas (%d x %d) ", CANVAS_WIDTH, CANVAS_HEIGHT);

    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            char ch = canvas[y][x];
            ShapeColor col = canvas_color[y][x];
            int col_pair = (int)col;
            if (col_pair < 0 || col_pair > 7) col_pair = 0;

            if (col_pair > 0 && has_colors()) {
                wattron(canvas_win, COLOR_PAIR(col_pair) | A_BOLD);
            }
            mvwaddch(canvas_win, y + 1, x + 1, ch);
            if (col_pair > 0 && has_colors()) {
                wattroff(canvas_win, COLOR_PAIR(col_pair) | A_BOLD);
            }
        }
    }
    wrefresh(canvas_win);
}

// Print color selections list inside menu window
void print_colors_in_menu(void) {
    wmove(menu_win, 7, 2);
    wclrtoeol(menu_win);
    wprintw(menu_win, "Colors: 0:Def, 1:Red, 2:Green, 3:Yellow, 4:Blue, 5:Mag, 6:Cyan, 7:Whi");
}

// Reset lines 5-11 inside the menu window box
void reset_menu_bottom(void) {
    for (int y = 5; y <= 11; y++) {
        wmove(menu_win, y, 1);
        wclrtoeol(menu_win);
    }
    box(menu_win, 0, 0); // Redraw border boundaries
}

// Print active shapes list in menu_win rows 1-3
void display_list_in_menu_win(void) {
    for (int y = 1; y <= 3; y++) {
        wmove(menu_win, y, 1);
        wclrtoeol(menu_win);
    }
    
    int row = 1;
    const char* col_names[] = {"Default", "Red", "Green", "Yellow", "Blue", "Magenta", "Cyan", "White"};

    for (int i = 0; i < MAX_SHAPES && row < 4; i++) {
        if (shapes[i].active) {
            ShapeColor color = shapes[i].color;
            int col_idx = (int)color;
            if (col_idx < 0 || col_idx > 7) col_idx = 0;

            if (col_idx > 0 && has_colors()) {
                wattron(menu_win, COLOR_PAIR(col_idx) | A_BOLD);
            }

            switch (shapes[i].type) {
                case SHAPE_LINE: {
                    LineParams lp = shapes[i].params.line;
                    mvwprintw(menu_win, row++, 2, "ID %-2d | Line: (%d,%d)-(%d,%d) | Char '%c' | Color: %s",
                              shapes[i].id, lp.start.x, lp.start.y, lp.end.x, lp.end.y,
                              shapes[i].draw_char, col_names[col_idx]);
                    break;
                }
                case SHAPE_RECTANGLE: {
                    RectParams rp = shapes[i].params.rect;
                    mvwprintw(menu_win, row++, 2, "ID %-2d | Rect: TL(%d,%d), w:%d, h:%d | Char '%c' | Color: %s",
                              shapes[i].id, rp.top_left.x, rp.top_left.y, rp.width, rp.height,
                              shapes[i].draw_char, col_names[col_idx]);
                    break;
                }
                case SHAPE_CIRCLE: {
                    CircleParams cp = shapes[i].params.circle;
                    mvwprintw(menu_win, row++, 2, "ID %-2d | Circle: Center(%d,%d), r:%d | Char '%c' | Color: %s",
                              shapes[i].id, cp.center.x, cp.center.y, cp.radius,
                              shapes[i].draw_char, col_names[col_idx]);
                    break;
                }
                case SHAPE_TRIANGLE: {
                    TriangleParams tp = shapes[i].params.triangle;
                    mvwprintw(menu_win, row++, 2, "ID %-2d | Tri: (%d,%d)-(%d,%d)-(%d,%d) | Char '%c' | Color: %s",
                              shapes[i].id, tp.p1.x, tp.p1.y, tp.p2.x, tp.p2.y, tp.p3.x, tp.p3.y,
                              shapes[i].draw_char, col_names[col_idx]);
                    break;
                }
            }

            if (col_idx > 0 && has_colors()) {
                wattroff(menu_win, COLOR_PAIR(col_idx) | A_BOLD);
            }
        }
    }
    if (row == 1) {
        mvwprintw(menu_win, 1, 2, "(No shapes added yet)");
    }
    
    mvwhline(menu_win, 4, 1, ACS_HLINE, CANVAS_WIDTH);
    wrefresh(menu_win);
}

// Interactive menu list selection controller (flicker-free)
int get_menu_selection_ncurses(WINDOW *win, int start_y, int start_x, const char *options[], int count, int current) {
    keypad(win, TRUE);
    int ch;
    while (1) {
        for (int i = 0; i < count; i++) {
            wmove(win, start_y + i, start_x);
            wclrtoeol(win);
            if (i == current) {
                wattron(win, A_REVERSE | COLOR_PAIR(6)); // Cyan highlight
                wprintw(win, " -> %s ", options[i]);
                wattroff(win, A_REVERSE | COLOR_PAIR(6));
            } else {
                wprintw(win, "    %s ", options[i]);
            }
        }
        wrefresh(win);

        ch = wgetch(win);
        switch (ch) {
            case KEY_UP:
            case 'w':
            case 'W':
            case 'i':
            case 'I':
                current = (current - 1 + count) % count;
                break;
            case KEY_DOWN:
            case 's':
            case 'S':
            case 'k':
            case 'K':
                current = (current + 1) % count;
                break;
            case 10:
            case KEY_ENTER:
                return current;
        }
    }
}

// Handle Add Shape flow
void ncurses_add_shape(void) {
    const char* add_menu_options[] = {
        "Draw Line",
        "Draw Rectangle",
        "Draw Circle",
        "Draw Triangle",
        "Cancel"
    };

    reset_menu_bottom();
    int select = get_menu_selection_ncurses(menu_win, 5, 2, add_menu_options, 5, 0);

    if (select == 4) return; // Cancelled

    char draw_char = '*';
    ShapeColor color = SHAPE_COLOR_DEFAULT;

    if (select == 0) { // Line
        int vals[4];
        reset_menu_bottom();
        if (!read_ints_ncurses(menu_win, 5, 2, "Enter coordinates x1 y1 x2 y2 (e.g. 5 3 25 10): ", vals, 4)) {
            mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
            wrefresh(menu_win);
            wgetch(menu_win);
            return;
        }
        clamp_point(&vals[0], &vals[1]);
        clamp_point(&vals[2], &vals[3]);
        
        read_char_ncurses(menu_win, 6, 2, "Enter drawing character (default '*'): ", &draw_char);
        print_colors_in_menu();
        int col_val[1] = {0};
        read_ints_ncurses(menu_win, 8, 2, "Select color ID (0-7): ", col_val, 1);
        if (col_val[0] >= 0 && col_val[0] <= 7) color = (ShapeColor)col_val[0];

        LineParams lp = {{vals[0], vals[1]}, {vals[2], vals[3]}};
        add_shape(SHAPE_LINE, &lp, draw_char, color);
    }
    else if (select == 1) { // Rectangle
        int vals[4];
        reset_menu_bottom();
        if (!read_ints_ncurses(menu_win, 5, 2, "Enter top-left x y width height (e.g. 10 3 15 6): ", vals, 4)) {
            mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
            wrefresh(menu_win);
            wgetch(menu_win);
            return;
        }
        clamp_point(&vals[0], &vals[1]);
        if (vals[2] < 1) vals[2] = 1;
        if (vals[3] < 1) vals[3] = 1;
        // Clamp rectangle bounds so it stays within canvas boundaries
        if (vals[0] + vals[2] > CANVAS_WIDTH) vals[2] = CANVAS_WIDTH - vals[0];
        if (vals[1] + vals[3] > CANVAS_HEIGHT) vals[3] = CANVAS_HEIGHT - vals[1];

        read_char_ncurses(menu_win, 6, 2, "Enter drawing character (default '*'): ", &draw_char);
        print_colors_in_menu();
        int col_val[1] = {0};
        read_ints_ncurses(menu_win, 8, 2, "Select color ID (0-7): ", col_val, 1);
        if (col_val[0] >= 0 && col_val[0] <= 7) color = (ShapeColor)col_val[0];

        RectParams rp = {{vals[0], vals[1]}, vals[2], vals[3]};
        add_shape(SHAPE_RECTANGLE, &rp, draw_char, color);
    }
    else if (select == 2) { // Circle
        int vals[3];
        reset_menu_bottom();
        if (!read_ints_ncurses(menu_win, 5, 2, "Enter center xc yc radius r (e.g. 30 6 4): ", vals, 3)) {
            mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
            wrefresh(menu_win);
            wgetch(menu_win);
            return;
        }
        clamp_point(&vals[0], &vals[1]);
        if (vals[2] < 1) vals[2] = 1;

        read_char_ncurses(menu_win, 6, 2, "Enter drawing character (default '*'): ", &draw_char);
        print_colors_in_menu();
        int col_val[1] = {0};
        read_ints_ncurses(menu_win, 8, 2, "Select color ID (0-7): ", col_val, 1);
        if (col_val[0] >= 0 && col_val[0] <= 7) color = (ShapeColor)col_val[0];

        CircleParams cp = {{vals[0], vals[1]}, vals[2]};
        add_shape(SHAPE_CIRCLE, &cp, draw_char, color);
    }
    else if (select == 3) { // Triangle
        int vals[6];
        reset_menu_bottom();
        if (!read_ints_ncurses(menu_win, 5, 2, "Enter vertices x1 y1 x2 y2 x3 y3: ", vals, 6)) {
            mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
            wrefresh(menu_win);
            wgetch(menu_win);
            return;
        }
        clamp_point(&vals[0], &vals[1]);
        clamp_point(&vals[2], &vals[3]);
        clamp_point(&vals[4], &vals[5]);

        read_char_ncurses(menu_win, 6, 2, "Enter drawing character (default '*'): ", &draw_char);
        print_colors_in_menu();
        int col_val[1] = {0};
        read_ints_ncurses(menu_win, 8, 2, "Select color ID (0-7): ", col_val, 1);
        if (col_val[0] >= 0 && col_val[0] <= 7) color = (ShapeColor)col_val[0];

        TriangleParams tp = {{vals[0], vals[1]}, {vals[2], vals[3]}, {vals[4], vals[5]}};
        add_shape(SHAPE_TRIANGLE, &tp, draw_char, color);
    }
}

// Handle Delete Shape flow
void ncurses_delete_shape(void) {
    reset_menu_bottom();
    int val[1];
    if (!read_ints_ncurses(menu_win, 5, 2, "Enter Shape ID to delete (or 0 to cancel): ", val, 1)) {
        mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }
    if (val[0] == 0) return;

    if (delete_shape(val[0])) {
        mvwprintw(menu_win, 6, 2, "Shape ID %d deleted successfully! Press any key...", val[0]);
    } else {
        mvwprintw(menu_win, 6, 2, "Shape ID %d not found or inactive. Press any key...", val[0]);
    }
    wrefresh(menu_win);
    wgetch(menu_win);
}

// Interactive Visual Shape Move tool (flicker-free with bounds lock)
void ncurses_move_shape(Shape *sh) {
    keypad(menu_win, TRUE);
    int ch;
    while (1) {
        render_all_shapes();
        display_canvas_ncurses();
        display_list_in_menu_win();

        reset_menu_bottom();
        mvwprintw(menu_win, 5, 2, "Moving Shape ID %d (%s)", sh->id, 
                  sh->type == SHAPE_LINE ? "Line" :
                  sh->type == SHAPE_RECTANGLE ? "Rect" :
                  sh->type == SHAPE_CIRCLE ? "Circle" : "Triangle");
        mvwprintw(menu_win, 6, 2, "Use Arrow Keys (or WASD/IJKL) to slide, Enter to Confirm.");
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        int dx = 0, dy = 0;
        
        switch (ch) {
            case KEY_UP:
            case 'w': case 'W': case 'i': case 'I':
                dy = -1;
                break;
            case KEY_DOWN:
            case 's': case 'S': case 'k': case 'K':
                dy = 1;
                break;
            case KEY_LEFT:
            case 'a': case 'A': case 'j': case 'J':
                dx = -1;
                break;
            case KEY_RIGHT:
            case 'd': case 'D': case 'l': case 'L':
                dx = 1;
                break;
            case 10:
            case KEY_ENTER:
                return; // Confirm and return
        }

        if (dx != 0 || dy != 0) {
            if (sh->type == SHAPE_LINE) {
                int new_x1 = sh->params.line.start.x + dx;
                int new_y1 = sh->params.line.start.y + dy;
                int new_x2 = sh->params.line.end.x + dx;
                int new_y2 = sh->params.line.end.y + dy;
                
                // Slide coordinates only if they stay within canvas limits
                if (new_x1 >= 0 && new_x1 < CANVAS_WIDTH && new_x2 >= 0 && new_x2 < CANVAS_WIDTH) {
                    sh->params.line.start.x = new_x1;
                    sh->params.line.end.x = new_x2;
                }
                if (new_y1 >= 0 && new_y1 < CANVAS_HEIGHT && new_y2 >= 0 && new_y2 < CANVAS_HEIGHT) {
                    sh->params.line.start.y = new_y1;
                    sh->params.line.end.y = new_y2;
                }
            }
            else if (sh->type == SHAPE_RECTANGLE) {
                int new_x = sh->params.rect.top_left.x + dx;
                int new_y = sh->params.rect.top_left.y + dy;
                
                if (new_x >= 0 && new_x + sh->params.rect.width <= CANVAS_WIDTH) {
                    sh->params.rect.top_left.x = new_x;
                }
                if (new_y >= 0 && new_y + sh->params.rect.height <= CANVAS_HEIGHT) {
                    sh->params.rect.top_left.y = new_y;
                }
            }
            else if (sh->type == SHAPE_CIRCLE) {
                int new_x = sh->params.circle.center.x + dx;
                int new_y = sh->params.circle.center.y + dy;
                
                // Allow center bounds movement
                if (new_x >= 0 && new_x < CANVAS_WIDTH) {
                    sh->params.circle.center.x = new_x;
                }
                if (new_y >= 0 && new_y < CANVAS_HEIGHT) {
                    sh->params.circle.center.y = new_y;
                }
            }
            else if (sh->type == SHAPE_TRIANGLE) {
                int new_x1 = sh->params.triangle.p1.x + dx;
                int new_y1 = sh->params.triangle.p1.y + dy;
                int new_x2 = sh->params.triangle.p2.x + dx;
                int new_y2 = sh->params.triangle.p2.y + dy;
                int new_x3 = sh->params.triangle.p3.x + dx;
                int new_y3 = sh->params.triangle.p3.y + dy;
                
                if (new_x1 >= 0 && new_x1 < CANVAS_WIDTH && 
                    new_x2 >= 0 && new_x2 < CANVAS_WIDTH && 
                    new_x3 >= 0 && new_x3 < CANVAS_WIDTH) {
                    sh->params.triangle.p1.x = new_x1;
                    sh->params.triangle.p2.x = new_x2;
                    sh->params.triangle.p3.x = new_x3;
                }
                if (new_y1 >= 0 && new_y1 < CANVAS_HEIGHT && 
                    new_y2 >= 0 && new_y2 < CANVAS_HEIGHT && 
                    new_y3 >= 0 && new_y3 < CANVAS_HEIGHT) {
                    sh->params.triangle.p1.y = new_y1;
                    sh->params.triangle.p2.y = new_y2;
                    sh->params.triangle.p3.y = new_y3;
                }
            }
        }
    }
}

// Live Visual Scaling/Zooming tool (+ for Zoom, - for Minimize)
void ncurses_scale_shape(Shape *sh) {
    keypad(menu_win, TRUE);
    int ch;
    while (1) {
        render_all_shapes();
        display_canvas_ncurses();
        display_list_in_menu_win();

        reset_menu_bottom();
        mvwprintw(menu_win, 5, 2, "Scaling Shape ID %d (%s)", sh->id, 
                  sh->type == SHAPE_LINE ? "Line" :
                  sh->type == SHAPE_RECTANGLE ? "Rect" :
                  sh->type == SHAPE_CIRCLE ? "Circle" : "Triangle");
        mvwprintw(menu_win, 6, 2, "Press '+' to Zoom, '-' to Minimize, Enter to Confirm.");
        wrefresh(menu_win);

        ch = wgetch(menu_win);
        if (ch == '+' || ch == '=') { // Zoom In
            if (sh->type == SHAPE_CIRCLE) {
                sh->params.circle.radius++;
            }
            else if (sh->type == SHAPE_RECTANGLE) {
                // Ensure rectangle resize doesn't stretch past canvas boundaries
                int x = sh->params.rect.top_left.x;
                int y = sh->params.rect.top_left.y;
                if (x + sh->params.rect.width + 1 <= CANVAS_WIDTH) sh->params.rect.width++;
                if (y + sh->params.rect.height + 1 <= CANVAS_HEIGHT) sh->params.rect.height++;
            }
            else if (sh->type == SHAPE_LINE) {
                int dx = sh->params.line.end.x - sh->params.line.start.x;
                int dy = sh->params.line.end.y - sh->params.line.start.y;
                
                int new_x = sh->params.line.end.x + ((dx > 0) ? 1 : (dx < 0 ? -1 : 0));
                int new_y = sh->params.line.end.y + ((dy > 0) ? 1 : (dy < 0 ? -1 : 0));
                if (new_x >= 0 && new_x < CANVAS_WIDTH) sh->params.line.end.x = new_x;
                if (new_y >= 0 && new_y < CANVAS_HEIGHT) sh->params.line.end.y = new_y;
            }
            else if (sh->type == SHAPE_TRIANGLE) {
                int dx2 = sh->params.triangle.p2.x - sh->params.triangle.p1.x;
                int dy2 = sh->params.triangle.p2.y - sh->params.triangle.p1.y;
                int dx3 = sh->params.triangle.p3.x - sh->params.triangle.p1.x;
                int dy3 = sh->params.triangle.p3.y - sh->params.triangle.p1.y;
                
                int n_x2 = sh->params.triangle.p2.x + ((dx2 > 0) ? 1 : (dx2 < 0 ? -1 : 0));
                int n_y2 = sh->params.triangle.p2.y + ((dy2 > 0) ? 1 : (dy2 < 0 ? -1 : 0));
                int n_x3 = sh->params.triangle.p3.x + ((dx3 > 0) ? 1 : (dx3 < 0 ? -1 : 0));
                int n_y3 = sh->params.triangle.p3.y + ((dy3 > 0) ? 1 : (dy3 < 0 ? -1 : 0));

                if (n_x2 >= 0 && n_x2 < CANVAS_WIDTH) sh->params.triangle.p2.x = n_x2;
                if (n_y2 >= 0 && n_y2 < CANVAS_HEIGHT) sh->params.triangle.p2.y = n_y2;
                if (n_x3 >= 0 && n_x3 < CANVAS_WIDTH) sh->params.triangle.p3.x = n_x3;
                if (n_y3 >= 0 && n_y3 < CANVAS_HEIGHT) sh->params.triangle.p3.y = n_y3;
            }
        }
        else if (ch == '-' || ch == '_') { // Minimize
            if (sh->type == SHAPE_CIRCLE) {
                if (sh->params.circle.radius > 1) sh->params.circle.radius--;
            }
            else if (sh->type == SHAPE_RECTANGLE) {
                if (sh->params.rect.width > 1) sh->params.rect.width--;
                if (sh->params.rect.height > 1) sh->params.rect.height--;
            }
            else if (sh->type == SHAPE_LINE) {
                int dx = sh->params.line.end.x - sh->params.line.start.x;
                int dy = sh->params.line.end.y - sh->params.line.start.y;
                if (abs(dx) > 1 || abs(dy) > 1) {
                    sh->params.line.end.x -= (dx > 0) ? 1 : (dx < 0 ? -1 : 0);
                    sh->params.line.end.y -= (dy > 0) ? 1 : (dy < 0 ? -1 : 0);
                }
            }
            else if (sh->type == SHAPE_TRIANGLE) {
                int dx2 = sh->params.triangle.p2.x - sh->params.triangle.p1.x;
                int dy2 = sh->params.triangle.p2.y - sh->params.triangle.p1.y;
                int dx3 = sh->params.triangle.p3.x - sh->params.triangle.p1.x;
                int dy3 = sh->params.triangle.p3.y - sh->params.triangle.p1.y;

                if (abs(dx2) > 1 || abs(dy2) > 1 || abs(dx3) > 1 || abs(dy3) > 1) {
                    sh->params.triangle.p2.x -= (dx2 > 0) ? 1 : (dx2 < 0 ? -1 : 0);
                    sh->params.triangle.p2.y -= (dy2 > 0) ? 1 : (dy2 < 0 ? -1 : 0);
                    sh->params.triangle.p3.x -= (dx3 > 0) ? 1 : (dx3 < 0 ? -1 : 0);
                    sh->params.triangle.p3.y -= (dy3 > 0) ? 1 : (dy3 < 0 ? -1 : 0);
                }
            }
        }
        else if (ch == 10 || ch == KEY_ENTER) {
            break; // Finished
        }
    }
}

// Handle Modify Shape properties
void ncurses_modify_shape(void) {
    reset_menu_bottom();
    int val[1];
    if (!read_ints_ncurses(menu_win, 5, 2, "Enter Shape ID to modify (or 0 to cancel): ", val, 1)) {
        mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }
    if (val[0] == 0) return;

    Shape* sh = find_shape(val[0]);
    if (sh == NULL) {
        mvwprintw(menu_win, 6, 2, "Shape ID %d not found. Press any key...", val[0]);
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }

    const char* modify_options[] = {
        "Move visually (Arrow Keys / WASD)",
        "Scale visually (Zoom '+' / Minimize '-')",
        "Edit coordinates manually",
        "Modify drawing character",
        "Modify color",
        "Cancel"
    };

    reset_menu_bottom();
    int choice = get_menu_selection_ncurses(menu_win, 5, 2, modify_options, 6, 0);

    if (choice == 5) return; // Cancelled

    if (choice == 0) { // Move visually
        ncurses_move_shape(sh);
    }
    else if (choice == 1) { // Scale visually
        ncurses_scale_shape(sh);
    }
    else if (choice == 2) { // Geometry manually
        reset_menu_bottom();
        if (sh->type == SHAPE_LINE) {
            int geom[4];
            mvwprintw(menu_win, 5, 2, "Current: (%d,%d) to (%d,%d)", 
                      sh->params.line.start.x, sh->params.line.start.y, sh->params.line.end.x, sh->params.line.end.y);
            if (read_ints_ncurses(menu_win, 6, 2, "Enter new start and end x1 y1 x2 y2: ", geom, 4)) {
                clamp_point(&geom[0], &geom[1]);
                clamp_point(&geom[2], &geom[3]);
                sh->params.line.start.x = geom[0];
                sh->params.line.start.y = geom[1];
                sh->params.line.end.x = geom[2];
                sh->params.line.end.y = geom[3];
                mvwprintw(menu_win, 7, 2, "Geometry modified successfully! Press any key...");
            } else {
                mvwprintw(menu_win, 7, 2, "Invalid input. Press any key...");
            }
        }
        else if (sh->type == SHAPE_RECTANGLE) {
            int geom[4];
            mvwprintw(menu_win, 5, 2, "Current: TL(%d,%d), w:%d, h:%d", 
                      sh->params.rect.top_left.x, sh->params.rect.top_left.y, sh->params.rect.width, sh->params.rect.height);
            if (read_ints_ncurses(menu_win, 6, 2, "Enter new x y w h: ", geom, 4)) {
                clamp_point(&geom[0], &geom[1]);
                if (geom[2] < 1) geom[2] = 1;
                if (geom[3] < 1) geom[3] = 1;
                if (geom[0] + geom[2] > CANVAS_WIDTH) geom[2] = CANVAS_WIDTH - geom[0];
                if (geom[1] + geom[3] > CANVAS_HEIGHT) geom[3] = CANVAS_HEIGHT - geom[1];

                sh->params.rect.top_left.x = geom[0];
                sh->params.rect.top_left.y = geom[1];
                sh->params.rect.width = geom[2];
                sh->params.rect.height = geom[3];
                mvwprintw(menu_win, 7, 2, "Geometry modified successfully! Press any key...");
            } else {
                mvwprintw(menu_win, 7, 2, "Invalid input. Press any key...");
            }
        }
        else if (sh->type == SHAPE_CIRCLE) {
            int geom[3];
            mvwprintw(menu_win, 5, 2, "Current: center (%d,%d), radius %d", 
                      sh->params.circle.center.x, sh->params.circle.center.y, sh->params.circle.radius);
            if (read_ints_ncurses(menu_win, 6, 2, "Enter new xc yc r: ", geom, 3)) {
                clamp_point(&geom[0], &geom[1]);
                if (geom[2] < 1) geom[2] = 1;

                sh->params.circle.center.x = geom[0];
                sh->params.circle.center.y = geom[1];
                sh->params.circle.radius = geom[2];
                mvwprintw(menu_win, 7, 2, "Geometry modified successfully! Press any key...");
            } else {
                mvwprintw(menu_win, 7, 2, "Invalid input. Press any key...");
            }
        }
        else if (sh->type == SHAPE_TRIANGLE) {
            int geom[6];
            mvwprintw(menu_win, 5, 2, "Current: p1(%d,%d), p2(%d,%d), p3(%d,%d)", 
                      sh->params.triangle.p1.x, sh->params.triangle.p1.y,
                      sh->params.triangle.p2.x, sh->params.triangle.p2.y,
                      sh->params.triangle.p3.x, sh->params.triangle.p3.y);
            if (read_ints_ncurses(menu_win, 6, 2, "Enter new vertices x1 y1 x2 y2 x3 y3: ", geom, 6)) {
                clamp_point(&geom[0], &geom[1]);
                clamp_point(&geom[2], &geom[3]);
                clamp_point(&geom[4], &geom[5]);

                sh->params.triangle.p1.x = geom[0];
                sh->params.triangle.p1.y = geom[1];
                sh->params.triangle.p2.x = geom[2];
                sh->params.triangle.p2.y = geom[3];
                sh->params.triangle.p3.x = geom[4];
                sh->params.triangle.p3.y = geom[5];
                mvwprintw(menu_win, 7, 2, "Geometry modified successfully! Press any key...");
            } else {
                mvwprintw(menu_win, 7, 2, "Invalid input. Press any key...");
            }
        }
        wrefresh(menu_win);
        wgetch(menu_win);
    }
    else if (choice == 3) { // Character
        reset_menu_bottom();
        char new_ch;
        mvwprintw(menu_win, 5, 2, "Current character: '%c'", sh->draw_char);
        if (read_char_ncurses(menu_win, 6, 2, "Enter new drawing character: ", &new_ch)) {
            sh->draw_char = new_ch;
            mvwprintw(menu_win, 7, 2, "Character modified successfully! Press any key...");
        } else {
            mvwprintw(menu_win, 7, 2, "Invalid input. Press any key...");
        }
        wrefresh(menu_win);
        wgetch(menu_win);
    }
    else if (choice == 4) { // Color
        reset_menu_bottom();
        print_colors_in_menu();
        int col_val[1] = {0};
        if (read_ints_ncurses(menu_win, 5, 2, "Select new color ID (0-7): ", col_val, 1)) {
            if (col_val[0] >= 0 && col_val[0] <= 7) {
                sh->color = (ShapeColor)col_val[0];
                mvwprintw(menu_win, 6, 2, "Color modified successfully! Press any key...");
            } else {
                mvwprintw(menu_win, 6, 2, "Invalid ID. Press any key...");
            }
        } else {
            mvwprintw(menu_win, 6, 2, "Invalid input. Press any key...");
        }
        wrefresh(menu_win);
        wgetch(menu_win);
    }
}

// Handle Background modification
void ncurses_canvas_settings(void) {
    reset_menu_bottom();
    mvwprintw(menu_win, 5, 2, "Current background: '%c'", bg_char);
    char new_bg;
    if (read_char_ncurses(menu_win, 6, 2, "Enter new background character: ", &new_bg)) {
        bg_char = new_bg;
        mvwprintw(menu_win, 7, 2, "Background updated successfully! Press any key...");
    } else {
        mvwprintw(menu_win, 7, 2, "Kept current background. Press any key...");
    }
    wrefresh(menu_win);
    wgetch(menu_win);
}

// Handle Export drawing layout
void ncurses_export_to_file(void) {
    reset_menu_bottom();
    wmove(menu_win, 5, 2);
    wprintw(menu_win, "Enter output filename (e.g. drawing.txt): ");
    wrefresh(menu_win);
    
    echo();
    curs_set(1);
    char filename[128];
    wgetnstr(menu_win, filename, sizeof(filename) - 1);
    noecho();
    curs_set(0);

    filename[strcspn(filename, "\n")] = '\0';
    if (strlen(filename) == 0) {
        mvwprintw(menu_win, 6, 2, "Export cancelled. Press any key...");
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        mvwprintw(menu_win, 6, 2, "Error: Could not open file '%s'. Press any key...", filename);
        wrefresh(menu_win);
        wgetch(menu_win);
        return;
    }

    render_all_shapes();
    for (int y = 0; y < CANVAS_HEIGHT; y++) {
        for (int x = 0; x < CANVAS_WIDTH; x++) {
            fputc(canvas[y][x], fp);
        }
        fputc('\n', fp);
    }
    fclose(fp);

    mvwprintw(menu_win, 6, 2, "Drawing layout exported to '%s'! Press any key...", filename);
    wrefresh(menu_win);
    wgetch(menu_win);
}

int main(void) {
    // Initial structures setup
    init_canvas('_');
    init_shapes();

    // Start ncurses screen mode
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    init_ncurses_colors();

    // Canvas occupies y=0 to y=13 (Height 14)
    // Menu occupies y=14 to y=26 (Height 13)
    canvas_win = newwin(CANVAS_HEIGHT + 2, CANVAS_WIDTH + 2, 0, 0);
    menu_win = newwin(13, CANVAS_WIDTH + 2, CANVAS_HEIGHT + 2, 0);

    const char *main_menu_options[] = {
        "Add Shape (Line, Rectangle, Circle, Triangle)",
        "Delete Shape (Remove by ID)",
        "Modify Shape (Move, Scale, Color, Char)",
        "Canvas Settings (Background character)",
        "Export Drawing to Text File",
        "Clear Canvas (Delete all shapes)",
        "Exit Program"
    };
    int choice = 0;

    while (1) {
        render_all_shapes();

        display_canvas_ncurses();
        display_list_in_menu_win();

        // Print Main Menu box header
        reset_menu_bottom();
        
        // Fetch index choice via arrow selection starting at row 5
        choice = get_menu_selection_ncurses(menu_win, 5, 2, main_menu_options, 7, choice);

        if (choice == 6) { // Exit Program
            break;
        }

        switch (choice) {
            case 0:
                ncurses_add_shape();
                break;
            case 1:
                ncurses_delete_shape();
                break;
            case 2:
                ncurses_modify_shape();
                break;
            case 3:
                ncurses_canvas_settings();
                break;
            case 4:
                ncurses_export_to_file();
                break;
            case 5: {
                reset_menu_bottom();
                char confirm = 'n';
                if (read_char_ncurses(menu_win, 5, 2, "Are you sure you want to delete all shapes? (y/n): ", &confirm) && (confirm == 'y' || confirm == 'Y')) {
                    init_shapes();
                    mvwprintw(menu_win, 6, 2, "Canvas cleared successfully! Press any key...");
                } else {
                    mvwprintw(menu_win, 6, 2, "Clear cancelled! Press any key...");
                }
                wrefresh(menu_win);
                wgetch(menu_win);
                break;
            }
        }
    }

    // End ncurses mode gracefully
    delwin(canvas_win);
    delwin(menu_win);
    endwin();

    printf("\nThank you for using the 2D Graphics Editor! Goodbye!\n\n");
    return 0;
}
