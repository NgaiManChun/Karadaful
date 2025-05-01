#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdlib.h>
#include "canvas.h"

static const char DEFAULT_PIXEL_CHAR[PIXEL_CHAR_SIZE] = "  ";
static char cell[PIXEL_CHAR_SIZE];
static char temp_cell[PIXEL_CHAR_SIZE * 2];

void createCanvas(int width, int height, CANVAS* canvas){
    canvas->width = width;
    canvas->height = height;
    if (width * height > 0) {
        canvas->pixels = (PIXEL*)malloc(sizeof(PIXEL) * width * height);
        if (canvas->pixels != NULL) {
            memset(canvas->pixels, 0, sizeof(PIXEL) * width * height);
            clearCanvas(canvas);
        }
    }
}

// BMPファイルの色の要素でキャンパスを作成
void createCanvasByBmp(const char* filename, CANVAS* canvas) {

    FILE* fp;
    fp = fopen(filename, "rb");
    if (fp != NULL) {

        unsigned char file_header[14];
        unsigned char info_headers[80];
        int* int_info_headers = (int *) &info_headers;
        short* short_info_headers = (short*)&info_headers;
        
        fread(&file_header, sizeof(char), 14, fp);
        fread(&info_headers, sizeof(int), 1, fp);
        fread((info_headers + 4), sizeof(char), int_info_headers[0] - sizeof(int), fp);

        int width = int_info_headers[1];
        int height = int_info_headers[2];
        createCanvas(width, height, canvas);

        // 一行のバイト数は４の倍数になるように
        // 余分データで埋まってるので注意
        int remaind = (4 - ((width * 3) % 4)) % 4;
        for (int y = height - 1; y >= 0; y--) {
            for (int x = 0; x < width; x++) {
                int index = width * y + x;
                fread(&canvas->pixels[index].bg_color.b, sizeof(unsigned char), 1, fp);
                fread(&canvas->pixels[index].bg_color.g, sizeof(unsigned char), 1, fp);
                fread(&canvas->pixels[index].bg_color.r, sizeof(unsigned char), 1, fp);
            }
            // 行の終わりに余分データをスキップ
            fseek(fp, sizeof(unsigned char) * remaind, SEEK_CUR);
        }

        fclose(fp);
        fp = NULL;
    }
    
}

void destoryCanvas(CANVAS *canvas) {
    if (canvas->pixels != NULL) {
        free(canvas->pixels);

        canvas->pixels = NULL;
    }
    canvas->width = 0;
    canvas->height = 0;
}

void clearCanvas(CANVAS* canvas) {
    for (int i = 0; i < canvas->width * canvas->height; i++) {
        canvas->pixels[i].color = COLOR_BLACK;
        canvas->pixels[i].bg_color = COLOR_BLACK;
        strcpy_s(canvas->pixels[i].character, DEFAULT_PIXEL_CHAR);
    }
}

bool isBlack(COLOR color) {
    return (color.r + color.g + color.b) == 0;
}

// 色をキャンパスに写す
void drawColorToCanvas(CANVAS* distination, int dx, int dy, int width, int height, COLOR color, COLOR bg_color, float a, float bg_a) {
    int offset_y = (dy < 0) ? -dy : 0;
    int offset_x_start = (dx < 0) ? -dx : 0;
    for (; offset_y < height; offset_y++) {
        // キャンパスの領域を出たら終わり
        if (dy + offset_y >= distination->height)
            break;

        int offset_x = (dx < 0) ? -dx : 0;
        for (int offset_x = offset_x_start; offset_x < width; offset_x++) {
            // キャンパスの領域を出たら終わり
            if (dx + offset_x >= distination->width)
                break;

            int d_index = (dy + offset_y) * distination->width + dx + offset_x;
            distination->pixels[d_index].color = colorComposition(distination->pixels[d_index].color, color, a);
            distination->pixels[d_index].bg_color = colorComposition(distination->pixels[d_index].bg_color, bg_color, bg_a);
        }
    }
}

// キャンパスをキャンパスに写す
void drawCanvasToCanvas(CANVAS* distination, CANVAS* source, int dx, int dy, float a, float bg_a, bool skip_text, bool skip_black_bg) {
    int offset_y = (dy < 0)?-dy :0;
    int offset_x_start = (dx < 0) ? -dx : 0;
    for (; offset_y < source->height; offset_y++) {
        // キャンパスの領域を出たら終わり
        if (dy + offset_y >= distination->height)
            break;

        for (int offset_x = offset_x_start; offset_x < source->width; offset_x++) {
            // キャンパスの領域を出たら終わり
            if (dx + offset_x >= distination->width)
                break;

            int s_index = offset_y * source->width + offset_x; // ソースのインデックス
            int d_index = (dy + offset_y) * distination->width + dx + offset_x; // 目的地のインデックス

            if (!skip_text) {
                strcpy_s(distination->pixels[d_index].character, source->pixels[s_index].character);
            }
            distination->pixels[d_index].color = colorComposition(distination->pixels[d_index].color, source->pixels[s_index].color, a);
            if (!(skip_black_bg && isBlack(source->pixels[s_index].bg_color))) {
                distination->pixels[d_index].bg_color = colorComposition(distination->pixels[d_index].bg_color, source->pixels[s_index].bg_color, bg_a);
            }
        }
    }
}

void drawBlockToCanvas(CANVAS* canvas, int x, int y, const char* block[], COLOR color, COLOR bg_color, float a, float bg_a) {
    int i = 0;
    while (block[i] != NULL) {
        drawStringToCanvas(canvas, x, y + i, block[i], color, bg_color, a, bg_a);
        i++;
    }
}

void drawStringToCanvas(CANVAS* canvas, int x, int y, const char* string, COLOR color, COLOR bg_color, float a, float bg_a) {

    int i = 0;
    while (string[i] != '\0') {
        temp_cell[i] = string[i];
        temp_cell[i + 1] = '\0';
        if (strlen(temp_cell) > 2) {
            temp_cell[i] = '\0';
            break;
        }
        i++;
    }
    while (strlen(temp_cell) < 2) {
        strcat_s(temp_cell, " ");
    }
    bool in_screen = x >= 0 && x < canvas->width;
    in_screen = in_screen && y >= 0 && y < canvas->height;
    if (in_screen) {
        int index = canvas->width * y + x;
        canvas->pixels[index].color = colorComposition(canvas->pixels[index].color, color, a);
        canvas->pixels[index].bg_color = colorComposition(canvas->pixels[index].bg_color, bg_color, bg_a);
        strcpy_s(canvas->pixels[index].character, temp_cell);

        if (string[i] != '\0') {
            drawStringToCanvas(canvas, x + 1, y, string + i, color, bg_color, a, bg_a);
        }
    }
}

COLOR colorComposition(COLOR c1, COLOR c2, float a) {
    unsigned char r = roundl(c2.r * a + c1.r * (1 - a));
    unsigned char g = roundl(c2.g * a + c1.g * (1 - a));
    unsigned char b = roundl(c2.b * a + c1.b * (1 - a));
    return {
        r,
        g,
        b
    };
}

COLOR hsv2rgb(float h, float s, float v) {
    float r = v;
    float g = v;
    float b = v;
    if (s > 0.0f) {
        h *= 6.0f;
        int i = (int)h;
        float f = h - (float)i;
        switch (i) {
        default:
        case 0:
            g *= 1 - s * (1 - f);
            b *= 1 - s;
            break;
        case 1:
            r *= 1 - s * f;
            b *= 1 - s;
            break;
        case 2:
            r *= 1 - s;
            b *= 1 - s * (1 - f);
            break;
        case 3:
            r *= 1 - s;
            g *= 1 - s * f;
            break;
        case 4:
            r *= 1 - s * (1 - f);
            g *= 1 - s;
            break;
        case 5:
            g *= 1 - s;
            b *= 1 - s * f;
            break;
        }
    }
    return { (unsigned char)roundl(r * 255), (unsigned char)roundl(g * 255), (unsigned char)roundl(b * 255) };
}