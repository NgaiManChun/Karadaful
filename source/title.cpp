#include "title.h"
#include "main.h"
#include "input_center.h"
#define CONIOEX
#include "conioex.h"

#define COLOR_PERIOD_TIME 1500
#define BGM_FILE "asset\\title_bgm.mp3"

static const SCENE THIS_SCENE = SCENE_TITLE;
static CANVAS canvas = { NULL, 0, 0 };
static CANVAS title_mask = {NULL, 0, 0};
static int bgm_handle = 0;

void titleInit() {
    // シーンキャンパス作成＆登録
    createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT, &canvas);
    setSceneCanvas(THIS_SCENE, &canvas);

    createCanvasByBmp("asset\\karadafuru.bmp", &title_mask);
    drawStringToCanvas(&title_mask, 16, 5, " Enterでスタート", COLOR_BLACK, COLOR_WHITE, 1.0f, 1.0f);
    drawStringToCanvas(&title_mask, 11, 12, "～お前もギタリストにしてやろうか～", COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
    bgm_handle = opensound((char *)BGM_FILE);
}

void titleDestory() {
    // シーンキャンパス削除
    destoryCanvas(&canvas);
    setSceneCanvas(THIS_SCENE, NULL);

    destoryCanvas(&title_mask);
    if (bgm_handle != 0) {
        closesound(bgm_handle);
        bgm_handle = 0;
    }
}

void titleScene() {
    
    if (!inSceneTransition() && isInputDown(INPUT_TYPE_ENTER)) {
        // Enterキーを押した時

        playsound(getSEHandle(SE_SUBMIT), 0);
        setTransition(SCENE_TRANSITATION_FILL_CIRCLE, DEFAULT_SCENE_TRANSITATION_DURTAION);
        callScene(SCENE_SONG_SELECT);
    }

    // タイトルの色を更新する
    float time = (timeGetTime() % COLOR_PERIOD_TIME) / (float)COLOR_PERIOD_TIME;
    for (int x = 0; x < title_mask.width; x++) {
        for (int y = 0; y < title_mask.height; y++) {
            int index = y * title_mask.width + x;

            // 黒の部分は変更しない
            bool is_black = ((int)title_mask.pixels[index].bg_color.r + title_mask.pixels[index].bg_color.g + title_mask.pixels[index].bg_color.b == 0);
            if (!is_black) {
                COLOR color = hsv2rgb(((int)(x + roundl(time * title_mask.width)) % title_mask.width) / (float)title_mask.width, 1.0f, 1.0f);
                title_mask.pixels[index].bg_color = color;
            }
        }
    }

    // BGMステータス更新
    if (bgm_handle != 0 && checksound(bgm_handle) == 0) {
        playsound(bgm_handle, 0);
    }

    // 描写
    clearCanvas(&canvas);
    drawCanvasToCanvas(&canvas, &title_mask, 0, 0, 1.0f, 1.0f, false, false);
    
}

