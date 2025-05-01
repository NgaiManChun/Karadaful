
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "conioex.h"

#include "input_center.h"
#include "song.h"
#include "main.h"
#include "canvas.h"
#include "title.h"
#include "song_select.h"
#include "game.h"

#define CAPTION "カラダ振る～お前もギタリストにしてやろうか～"

static SCENE active_scene = SCENE_NONE; // 現在のシーン
static SCENE standby_scene = SCENE_NONE; // シーン遷移先
static CANVAS *scene_canvas_array[SCENE_COUNT]; // 各シーンの独立キャンパスのポインタの格納場所
static CANVAS canvas = {NULL, 0, 0}; // ゲーム全体のメインキャンパス
static CANVAS none_scene_canvas = { NULL, 0, 0 }; // 予備用のシーン遷移元
 
static bool in_scene_transitaion = false;
static SCENE_TRANSITATION scene_transitation = SCENE_TRANSITATION_TRANSPARENT; // シーン遷移のタイプ
static int scene_transitation_duration = DEFAULT_SCENE_TRANSITATION_DURTAION; // シーン遷移所要時間
static float scene_transitation_progress = 0.0f; // シーン遷移の進捗
static COLOR fill_color = COLOR_BLACK; // シーン遷移用色

static int deltaTime = 0; // 前フレームから現フレームの間の経過時間

// １マスを表現する文字列のバイト数の最大値
static const int OUTPUT_CELL_SIZE = sizeof("\033[48;2;255;255;255m\033[38;2;255;255;255m") + PIXEL_CHAR_SIZE;

// １マスを表現する文字列の一時格納場所
static char cell[OUTPUT_CELL_SIZE]; 

// 全画面を表現する文字列
static char output_string[SCREEN_WIDTH * SCREEN_HEIGHT * (OUTPUT_CELL_SIZE)+sizeof("\033[0m") + 1];

static SCENE_STATE scene_state = SCENE_STATE_CONTINUE;

static const char* se_files[] = {
    "asset\\change.mp3",
    "asset\\submit.mp3",
    "asset\\cancel.mp3"
};
static int se_handles[_countof(se_files)];

// フレームの最小間隔（ms）
const float frame_span = 1000.0f / FRAME_RATE;

void initWindow();
void updateSceneTransitaion();
void drawSceneTransitation();
void doScene();
void updateOutputString(CANVAS* canvas);

int main() {
    // エスケープシーケンス使用開始
    system("cls");

    // ウィンドウタイトル設定
    setcaption((char *)CAPTION);

    // カーソル消し
    setcursortype(NOCURSOR);

    // ウィンドウサイズ設定
    initWindow();

    // 分解能設定
    timeBeginPeriod(1);

    // メインキャンパス準備
    createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT, &canvas);

    // 空キャンパス準備
    createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT, &none_scene_canvas);

    // SE初期化
    memset(se_handles, 0, sizeof(int) * _countof(se_files));
    for (int i = 0; i < _countof(se_files); i++) {
        se_handles[i] = opensound((char*)se_files[i]);
    }

    // ソングリスト初期化
    loadSongList();

    // 最初だけ空キャンパスをセット
    setSceneCanvas(SCENE_NONE, &none_scene_canvas);

    // タイトルシーンから始まる
    callScene(SCENE_TITLE);

    int lastFrameTime = timeGetTime();
    int currentTime;

    while (scene_state != SCENE_STATE_END) {

        // 前フレームからの経過時間を取得
        currentTime = timeGetTime();
        deltaTime = currentTime - lastFrameTime;

        if (deltaTime >= frame_span) {

            // 入力情報更新
            recordInputs();
            
            // シーン遷移状況を更新
            updateSceneTransitaion();

            // 現シーン（及び遷移先のシーン）の処理
            doScene();
            
            if (in_scene_transitaion) {
                // シーン遷移の描画処理
                drawSceneTransitation();
            }
            else {
                // シーンキャンパスをメインキャンパスに写す
                drawCanvasToCanvas(&canvas, getSceneCanvas(active_scene), 0, 0, 1.0f, 1.0f, false, false);
            }

            // メインキャンパスのデータから描画文字列を作成
            updateOutputString(&canvas);

            // 描画文字列をプリントアウト
            gotoxy(1, 1);
            printf_s(output_string);

            // 前フレーム時間更新
            lastFrameTime = currentTime;
        }

    }

    // 色々後処理
    destoryCanvas(&canvas);
    destoryCanvas(&none_scene_canvas);
    for (int i = 0; i < _countof(se_files); i++) {
        closesound(se_handles[i]);
        se_handles[i] = 0;
    }
    destorySongList();

    return 0;
}

void updateSceneTransitaion() {
    if (in_scene_transitaion) {
        if (scene_transitation_progress >= 1.0f) {
            // シーン遷移完了時
            // シーン交代
            destoryScene(active_scene);
            active_scene = standby_scene;
            standby_scene = SCENE_NONE;
            in_scene_transitaion = false;
            scene_transitation_progress = 0.0f;

        }
        else {
            // シーン遷移途中
            scene_transitation_progress = updateProgress(scene_transitation_progress, (float)deltaTime, scene_transitation_duration);
        }
    }
}

void drawSceneTransitation() {
    if (scene_transitation == SCENE_TRANSITATION_TRANSPARENT) {
        // 透かしタイプ

        drawCanvasToCanvas(&canvas, getSceneCanvas(active_scene), 0, 0, 1.0f - scene_transitation_progress, 1.0f - scene_transitation_progress, false, false);
        drawCanvasToCanvas(&canvas, getSceneCanvas(standby_scene), 0, 0, scene_transitation_progress, scene_transitation_progress, false, false);
    }
    else if (scene_transitation == SCENE_TRANSITATION_FILL_CIRCLE) {
        // 波紋タイプ
        

        // 全画面の対角線の長さ
        float max_r = sqrtf(SCREEN_WIDTH * SCREEN_WIDTH + SCREEN_HEIGHT * SCREEN_HEIGHT);

        // 波紋の外側の最大半径
        float wave_max_r = max_r * 1.2f;

        // 遷移先のキャンパス
        CANVAS* standby_canvas = getSceneCanvas(standby_scene);

        // まずは通常通りメインキャンパスに写す
        drawCanvasToCanvas(&canvas, getSceneCanvas(active_scene), 0, 0, 1.0f, 1.0f, false, false);

        for (int y = 0; y < canvas.height; y++) {
            for (int x = 0; x < canvas.width; x++) {
                // 左上から現ピクセルの距離
                float distance = fabsf(sqrtf(x * x + y * y));
                
                float factor1 = distance / (scene_transitation_progress * wave_max_r);
                if (factor1 <= 1.0f) {
                    // 波紋内側

                    int index = y * SCREEN_WIDTH + x;

                    // 遷移先の内容を写す
                    canvas.pixels[index].bg_color = colorComposition(canvas.pixels[index].bg_color, standby_canvas->pixels[index].bg_color, 1.0f);
                    canvas.pixels[index].color = colorComposition(canvas.pixels[index].color, standby_canvas->pixels[index].color, 1.0f);
                    strcpy_s(canvas.pixels[index].character, standby_canvas->pixels[index].character);

                    // 波紋のエッジを描写
                    canvas.pixels[index].bg_color = colorComposition(canvas.pixels[index].bg_color, fill_color, factor1 * (1.0f - scene_transitation_progress));
                    canvas.pixels[index].color = colorComposition(canvas.pixels[index].color, fill_color, factor1 * (1.0f - scene_transitation_progress));
                }
            }
        }
    }
}

void doScene() {
    SCENE scenes[] = { active_scene , standby_scene };
    for (int i = 0; i < _countof(scenes); i++) {
        if (scenes[i] == SCENE_TITLE) {
            titleScene();
        }
        else if (scenes[i] == SCENE_SONG_SELECT) {
            songSelectScene();
        }
        else if (scenes[i] == SCENE_GAME) {
            gameScene();
        }
    }
}

int getSEHandle(SE se){
    return se_handles[se];
}

void setSceneCanvas(SCENE scene, CANVAS* canvas) {
    scene_canvas_array[scene] = canvas;
}

CANVAS* getSceneCanvas(SCENE scene) {
    return scene_canvas_array[scene];
}

void initScene(SCENE scene) {
    if (scene == SCENE_TITLE) {
        titleInit();
    }
    else if (scene == SCENE_SONG_SELECT) {
        songSelectInit();
    }
    else if (scene == SCENE_GAME) {
        gameInit();
    }
}

void destoryScene(SCENE scene) {
    if (scene == SCENE_TITLE) {
        titleDestory();
    }
    else if (scene == SCENE_SONG_SELECT) {
        songSelectDestory();
    }
    else if (scene == SCENE_GAME) {
        gameDestory();
    }
}

// シーンの呼び出し
void callScene(SCENE scene) {

    // 前の遷移先のシーンの後処理(空の場合あり)
    destoryScene(standby_scene);

    // 呼び出しシーンの初期化
    initScene(scene);

    // 呼び出しシーンをシーン遷移先としてセット
    standby_scene = scene;

    // シーン遷移進捗リセット
    in_scene_transitaion = true;
    scene_transitation_progress = 0.0f;
    
    // ランダムの遷移効果の色
    fill_color = hsv2rgb((timeGetTime() % 100) / 100.0f, 0.3f, 1.0f);
}

bool inSceneTransition() {
    return in_scene_transitaion;
}

void setTransition(SCENE_TRANSITATION transitation, int duration) {
    scene_transitation = transitation;
    if (duration >= 0) {
        scene_transitation_duration = duration;
    }
}

int getDeltaTime() {
    return deltaTime;
}

// ピクセルデータから表示用の文字列を作成
void updateOutputString(CANVAS *canvas) {
    // 前の文字列をクリア
    output_string[0] = '\0';

    int num_of_pixel = canvas->width * canvas->height;
    for (int i = 0; i < num_of_pixel; i++) {
        cell[0] = '\0';
        sprintf_s(cell, "\033[48;2;%d;%d;%dm\033[38;2;%d;%d;%dm%s",
            canvas->pixels[i].bg_color.r, canvas->pixels[i].bg_color.g, canvas->pixels[i].bg_color.b,
            canvas->pixels[i].color.r, canvas->pixels[i].color.g, canvas->pixels[i].color.b,
            canvas->pixels[i].character);
        strcat_s(output_string, cell);
    }

    // 最後にコンソールの色設定をリセット
    strcat_s(output_string, "\033[0m");
}

// ウインドウサイズ初期化
void initWindow() {
    HWND consoleWindow = GetConsoleWindow();
    SetWindowPos(consoleWindow, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    // Set console window size
    SMALL_RECT windowSize = { 0, 0, 80, 25 };
    SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &windowSize);

    // Set console buffer size
    COORD coord;
    coord.X = 80;
    coord.Y = 25;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coord);

    // Set console font style and size
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 15;                   // Width of each character in the font
    cfi.dwFontSize.Y = 30;                   // Height
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"MS Gothic");       // Choose your font
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

}

// ===== 汎用関数　開始 =====
int nextIndex(int index, int limit) {
    return (index + 1) % limit;
}

int previousIndex(int index, int limit) {
    return (--index < 0) ? index + limit : index;
}

float updateProgress(float current, float deltaTime, float duration) {
    current += deltaTime / duration;
    if (current > 1.0f)current = 1.0f;
    return current;
}
// ===== 汎用関数　終了 =====

