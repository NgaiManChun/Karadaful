#include <string.h>
#ifndef _CANVAS_H
#define _CANVAS_H

typedef struct _COLOR {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} COLOR;

#define PIXEL_CHAR_SIZE 6
#define COLOR_WHITE { 255, 255, 255 }
#define COLOR_BLACK { 0, 0, 0 }

typedef struct _PIXEL {
    COLOR color;
    COLOR bg_color;
    char character[PIXEL_CHAR_SIZE] = "";
} PIXEL;

typedef struct _CANVAS {
    PIXEL* pixels = NULL;
    int width;
    int height;
} CANVAS;

void createCanvas(int width, int height, CANVAS *canvas);
void createCanvasByBmp(const char* filename, CANVAS* canvas);
void destoryCanvas(CANVAS* canvas);
void clearCanvas(CANVAS* canvas);
bool isBlack(COLOR color);
void drawColorToCanvas(CANVAS* distination, int dx, int dy, int width, int height, COLOR color, COLOR bg_color, float a, float bg_a);
void drawStringToCanvas(CANVAS* canvas, int x, int y, const char* string, COLOR color, COLOR bg_color, float a, float bg_a);
void drawBlockToCanvas(CANVAS* canvas, int x, int y, const char* block[], COLOR color, COLOR bg_color, float a, float bg_a);
void drawCanvasToCanvas(CANVAS* distination, CANVAS* source, int dx, int dy, float a, float bg_a, bool skip_text, bool skip_black_bg);
COLOR colorComposition(COLOR c1, COLOR c2, float a);
COLOR hsv2rgb(float h, float s, float v);

#endif