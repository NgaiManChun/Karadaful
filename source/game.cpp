
#include "game.h"
#include "song.h"
#include "main.h"
#include "input_center.h"
#define CONIOEX
#include "conioex.h"

#define START_MARGIN_TIME 1000
#define END_MARGIN_TIME 1000
#define BORDER_COLOR { 125, 125, 125 }
#define TRACK_HEIGHT 2
#define VERTICAL_LINE_X 4
#define ATTACK_DURATION 500
#define ATTACK_MARGIN 2
#define RESULT_MARGIN_X 2
#define RESULT_MARGIN_Y 2
#define RESULT_PADDING_X 1
#define RESULT_PADDING_Y 1

#define COMBO_ADD_SCALE (0.01f)

#define COMBO_FILE "asset\\combo.bmp"
#define COMBO_ANIMATION_DURATION 300

#define BG_FRAME_DURATION 1000
#define BG_FRAME_COUNT 2

#define ATTACK_BASIC_SCORE 100
#define SUSTAIN_BASIC_SCORE 20
#define NOISE_PENALTY_SCALE (0.33f)

#define RETIRE_DURATION (1000.0f)

#define RESULT_BGM_FILE "asset\\result_bgm.mp3"

enum ATTACK_QUALITY {
    ATTACK_QUALITY_PERFECT,
    ATTACK_QUALITY_GREAT,
    ATTACK_QUALITY_COOL,
    ATTACK_QUALITY_GOOD,
    ATTACK_QUALITY_BAD,
    ATTACK_QUALITY_NONE
};

struct ATTACK {
    int pitch;
    int time;
    ATTACK_QUALITY attack_quality = ATTACK_QUALITY_NONE;
};

static INPUT_TYPE pitch_keys[] = {
    INPUT_TYPE_F1,
    INPUT_TYPE_F2,
    INPUT_TYPE_F3,
    INPUT_TYPE_F4,
    INPUT_TYPE_F5,
    INPUT_TYPE_F6
};

static const char pitch_labels[][3] = {
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6"
};

static const char quality_labels[][8] = {
    "PERFECT",
    "GREAT",
    "COOL",
    "GOOD",
    "BAD"
};

static const char* number_files[] = {
    "asset\\number0.bmp",
    "asset\\number1.bmp",
    "asset\\number2.bmp",
    "asset\\number3.bmp",
    "asset\\number4.bmp",
    "asset\\number5.bmp",
    "asset\\number6.bmp",
    "asset\\number7.bmp",
    "asset\\number8.bmp",
    "asset\\number9.bmp"
};
static const char* fever_bg_files[BG_FRAME_COUNT] = { "asset\\fire1.bmp", "asset\\fire2.bmp" };

static const char* rank_files[] = {
    "asset\\s.bmp",
    "asset\\a.bmp",
    "asset\\b.bmp",
    "asset\\c.bmp",
    "asset\\d.bmp",
    "asset\\e.bmp"
};

static const char* pressed_mark[] = {
    "▼",
    "▲",
    NULL
};

const char* hints = "弾く：F1～F6 + Enter";

static const SCENE THIS_SCENE = SCENE_GAME;
static CANVAS canvas = { NULL, 0, 0 };
static CANVAS borders = { NULL, 0, 0 };
static CANVAS combo_canvas = { NULL, 0, 0 };
static CANVAS number_canvas[10];
static CANVAS rank_canvas[_countof(rank_files)];
static CANVAS fever_bg_canvas[BG_FRAME_COUNT];
static bool isRinging = false;
static int current_time = 0;
static int start_margin_time = START_MARGIN_TIME;
static int end_margin_time = END_MARGIN_TIME;
static unsigned short* timeline;
static int score = 0;
static int combo = 0;
static int max_combo = 0;
static int basic_full_score = 0;
static ATTACK pitch_attack_quality[TRACK_COUNT];
static COLOR pitch_effect_colors[TRACK_COUNT];
static COLOR pitch_notes_colors[TRACK_COUNT];
static int quality_counts[ATTACK_QUALITY_NONE];
static int sound_delay = 0;

static SONG song;
static int audio_handle;
static int result_bgm_handle;

static float combo_animation_progress = 1.0f;
static float scroll_speed = 1.0f;
static int simple_per_pixel = 0;

static int fever_bg_animation_time = 0;
static int fever_bg_frame_index = 0;
static float retire_progress = 0.0f;
static char str_buffer[256];

void gameSetSong(SONG _song) {
    song = _song;
}

void gameSetScrollSpeed(float _scroll_speed) {
    scroll_speed = _scroll_speed;
}

void gameSetSoundDelay(int _sound_delay) {
    sound_delay = _sound_delay;
}

void gameInit() {
    // シーンキャンパス作成＆登録
    createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT, &canvas);
    setSceneCanvas(THIS_SCENE, &canvas);

    score = 0;
    combo = 0;
    max_combo = 0;
    combo_animation_progress = 1.0f;
    retire_progress = 0.0f;
    current_time = 0;
    start_margin_time = START_MARGIN_TIME;
    end_margin_time = END_MARGIN_TIME;
    memset(quality_counts, 0, sizeof(int) * ATTACK_QUALITY_NONE);
    
    for (int i = 0; i < TRACK_COUNT; i++) {
        pitch_attack_quality[i].time = -ATTACK_DURATION;
        pitch_attack_quality->attack_quality = ATTACK_QUALITY_NONE;
        pitch_effect_colors[i] = hsv2rgb((1.0f / TRACK_COUNT) * i, 0.6f, 1.0f);
        pitch_notes_colors[i] = hsv2rgb((1.0f / TRACK_COUNT) * i, 0.4f, 1.0f);
    }

    // 枠キャンパスを作成
    createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT, &borders);
    for (int i = 0; i < TRACK_COUNT; i++) {
        int y = i * TRACK_HEIGHT + i;
        for (int x = 0; x < borders.width; x++) {
            drawStringToCanvas(&borders, x, y, "―", BORDER_COLOR, COLOR_BLACK, 1.0f, 0.0f);
            drawStringToCanvas(&borders, x, y + TRACK_HEIGHT + 1, "―", BORDER_COLOR, COLOR_BLACK, 1.0f, 0.0f);
        }
    }
    for (int y = 1; y < TRACK_COUNT * TRACK_HEIGHT + TRACK_COUNT; y++) {
        drawStringToCanvas(&borders, VERTICAL_LINE_X, y, "｜", BORDER_COLOR, COLOR_BLACK, 1.0f, 0.0f);
    }

    // コンボキャンパスを作成
    createCanvasByBmp(COMBO_FILE, &combo_canvas);
    for (int y = 0; y < combo_canvas.height; y++) {
        for (int x = 0; x < combo_canvas.width; x++) {
            int index = y * combo_canvas.width + x;
            bool is_black = (combo_canvas.pixels[index].bg_color.r + combo_canvas.pixels[index].bg_color.g + combo_canvas.pixels[index].bg_color.b) == 0;
            if (!is_black) {
                COLOR color = hsv2rgb((float)x / combo_canvas.width, 1.0f, 1.0f);
                combo_canvas.pixels[index].bg_color = colorComposition(combo_canvas.pixels[index].bg_color, color, 0.5f);
                combo_canvas.pixels[index].color = combo_canvas.pixels[index].bg_color;
            }

        }
    }
    
    // 数字キャンパスを作成
    for (int i = 0; i < 10; i++) {
        createCanvasByBmp(number_files[i], &number_canvas[i]);
        for (int y = 0; y < number_canvas[i].height; y++) {
            for (int x = 0; x < number_canvas[i].width; x++) {
                int index = y * number_canvas[i].width + x;
                bool is_black = (number_canvas[i].pixels[index].bg_color.r + number_canvas[i].pixels[index].bg_color.g + number_canvas[i].pixels[index].bg_color.b) == 0;
                if (!is_black) {
                    COLOR color = hsv2rgb(i / 10.0f, 0.5f, 1.0f);
                    number_canvas[i].pixels[index].bg_color = colorComposition(number_canvas[i].pixels[index].bg_color, color, 1.0f);
                    number_canvas[i].pixels[index].color = number_canvas[i].pixels[index].bg_color;
                }

            }
        }
    }

    // ランクキャンパスを作成
    for (int i = 0; i < _countof(rank_files); i++) {
        createCanvasByBmp(rank_files[i], &rank_canvas[i]);
    }
    
    // ビーバー時の背景
    for (int i = 0; i < BG_FRAME_COUNT; i++) {
        createCanvasByBmp(fever_bg_files[i], &fever_bg_canvas[i]);
    }
    
    // タイムラインを作成
    long simple_num = SIMPLING_RATE * song.length;
    timeline = (unsigned short *)malloc(sizeof(unsigned short) * simple_num);
    if (timeline) {
        memset(timeline, 0, sizeof(unsigned short) * simple_num);
    }

    // テンポグループを読み込む、ノーツのデータを書き込む
    loadTempoGroups(song.tempo_groups_file, timeline, song.length);

    // 横軸に１ピクセルはタイムラインのサンプル何個分に当たるかの数字
    simple_per_pixel = roundl(song.basic_view_second / scroll_speed * SIMPLING_RATE / SCREEN_WIDTH);

    // コンボボーナス抜きのフルスコアを割り出す
    basic_full_score = 0;
    if (timeline) {
        int quality_scale = ATTACK_QUALITY_BAD - ATTACK_QUALITY_PERFECT;
        for (long i = 0; i < simple_num; i++) {
            for (int pitch = 0; pitch < TRACK_COUNT; pitch++) {
                if (timeline[i] >> (pitch * 2) & NOTE_START) basic_full_score += ATTACK_BASIC_SCORE * quality_scale;
                if (timeline[i] >> (pitch * 2) & NOTE_SUSTAIN) basic_full_score += SUSTAIN_BASIC_SCORE;
            }
        }
    }
    
    // 曲のオーディオファイルを展開
    audio_handle = opensound(song.audio_file);
    setvolume(audio_handle, 100);

    // リザルトBGMを展開
    result_bgm_handle = opensound((char*)RESULT_BGM_FILE);

}

void gameDestory() {
    destoryCanvas(&canvas);
    setSceneCanvas(THIS_SCENE, NULL);

    // 枠キャンパス廃棄
    destoryCanvas(&borders);

    // コンボキャンパス廃棄
    destoryCanvas(&combo_canvas);

    // 数字キャンパス廃棄
    for (int i = 0; i < 10; i++) {
        destoryCanvas(&number_canvas[i]);
    }

    // ランクキャンパスの作成
    for (int i = 0; i < _countof(rank_files); i++) {
        destoryCanvas(&rank_canvas[i]);
    }

    // ビーバー背景廃棄
    for (int i = 0; i < BG_FRAME_COUNT; i++) {
        destoryCanvas(&fever_bg_canvas[i]);
    }

    // タイムライン廃棄
    free(timeline);
    timeline = 0;

    // オーディオハンドル等廃棄
    if (audio_handle != 0) {
        closesound(audio_handle);
        audio_handle = 0;
    }
    if (result_bgm_handle != 0) {
        closesound(result_bgm_handle);
        result_bgm_handle = 0;
    }
    
}

// アタック（ノーツタッチ）のクオリティごとの色を取得
// PERFECT、GREATの色点滅するので、毎フレーム新しく取得をおススメ
COLOR getQualityLabelColor(ATTACK_QUALITY quality) {
    if (quality == ATTACK_QUALITY_PERFECT) {
        return hsv2rgb((current_time % 500) / 500.0f, 0.5f, 1.0f);
    }
    else if (quality == ATTACK_QUALITY_GREAT) {
        return hsv2rgb((current_time % 2000) / 2000.0f, 0.3f, 0.75f);
    }
    else if (quality == ATTACK_QUALITY_COOL) {
        return hsv2rgb(0.7f, 0.5f, 1.0f);
    }
    else if (quality == ATTACK_QUALITY_GOOD) {
        return { 186, 186, 186 };
    }
    else if (quality == ATTACK_QUALITY_BAD) {
        return { 70,70,70 };
    }
    return COLOR_WHITE;
}

// 特定のピッチ（ノーツトラック）のアタック（タッチ）状態
bool isPitchAttacking(int pitch) {
    return (isRinging && isInputDown(pitch_keys[pitch])) || (isInputDown(INPUT_TYPE_ENTER) && isInputHold(pitch_keys[pitch]));
}

// サンプルのインデックスがタイムラインからはみ出さないようにするための汎用関数
long limitedSampleIndex(long index) {
    if (index < 0)
        return 0;
    if (index >= SIMPLING_RATE * song.length)
        return SIMPLING_RATE * song.length - 1;
    return index;
}

void gameScene() {
    bool isEnd = current_time > song.length * 1000 + end_margin_time;

    // 任意のピッチを鳴らしてるかのチェック
    bool _isRinging = false;
    for (int i = 0; i < TRACK_COUNT; i++) {
        if (isPitchAttacking(i) || (isRinging && isInputHold(pitch_keys[i]))) {
            _isRinging = true;
            break;
        }
    }
    isRinging = _isRinging;

    // Esc長押しのカウント更新
    if (!isEnd && isInputHold(INPUT_TYPE_ESC)) {
        retire_progress = updateProgress(retire_progress, (float)getDeltaTime(), RETIRE_DURATION);
    }
    else if(retire_progress <= 1.0f){
        retire_progress = 0.0f;
    }

    // リタイヤ処理
    if (!inSceneTransition() && retire_progress >= 1.0f) {
        stopsound(audio_handle);
        playsound(getSEHandle(SE_CANCEL), 0);

        if (audio_handle != 0) {
            closesound(audio_handle);
        }
        audio_handle = 0;

        callScene(SCENE_SONG_SELECT);
    }

    // 現フレームのサンプル
    long current_simple = limitedSampleIndex(floorl((current_time - sound_delay) / 1000.0f * SIMPLING_RATE));

    // 前フレームのサンプル
    long perivous_simple = limitedSampleIndex(floorl((current_time - sound_delay - getDeltaTime()) / 1000.0f * SIMPLING_RATE));

    // アタック最小有効サンプル
    long attack_min_simple = limitedSampleIndex(current_simple - ATTACK_QUALITY_BAD * ATTACK_MARGIN);

    // アタック最大有効サンプル
    long attack_max_simple = limitedSampleIndex(current_simple + ATTACK_QUALITY_BAD * ATTACK_MARGIN);

    // ミス判定のサンプル、前のフレームからアタック最小有効サンプルまで
    long bad_simple = limitedSampleIndex(attack_min_simple - floorl(getDeltaTime() / 1000.0f * SIMPLING_RATE));

    // コンボ得点ボーナス
    long combo_score_scale = (1.0f + combo * COMBO_ADD_SCALE);

    // タッチ判定
    if (!isEnd) {
        int add_score = 0;
        int noise_count = 0; // 余分なタッチ
        for (int pitch = 0; pitch < TRACK_COUNT; pitch++) {

            // ミス判定
            for (long i = bad_simple; i <= attack_min_simple; i++) {
                if ((timeline[i] >> pitch * 2) & NOTE_START) {
                    pitch_attack_quality[pitch] = { pitch, current_time, ATTACK_QUALITY_BAD };
                    quality_counts[ATTACK_QUALITY_BAD]++;
                    combo = 0;
                }
            }

            bool isNoise = false;
            if (isPitchAttacking(pitch) || isRinging && isInputHold(pitch_keys[pitch])) {
                isNoise = true;
            }

            // アタック判定、ノーツ頭
            if (isPitchAttacking(pitch)) {
                ATTACK_QUALITY attack_quality = ATTACK_QUALITY_NONE;
                for (long i = attack_min_simple + 1; i < attack_max_simple; i++) {
                    if ((timeline[i] >> pitch * 2) & NOTE_START) {
                        attack_quality = (ATTACK_QUALITY)floor(abs(current_simple - i) / ATTACK_MARGIN);
                        pitch_attack_quality[pitch] = { pitch, current_time, attack_quality };
                        timeline[i] = timeline[i] & ~(NOTE_START << pitch * 2);
                        quality_counts[attack_quality]++;
                        int quality_scale = ATTACK_QUALITY_BAD - attack_quality;
                        combo++;
                        combo_animation_progress = 0.0f;
                        add_score += ATTACK_BASIC_SCORE * quality_scale * combo_score_scale;
                        isNoise = false;
                        break;
                    }
                }

                // 有効範囲内に過ぎ去ったサスティンを消す、得点はなし
                for (long i = attack_min_simple + 1; i < current_simple; i++) {
                    if ((timeline[i] >> pitch * 2) & NOTE_SUSTAIN) {
                        timeline[i] = timeline[i] & ~(NOTE_SUSTAIN << pitch * 2);
                        isNoise = false;
                    }
                }
                
            }
            
            // サスティン判定
            if (isRinging && isInputHold(pitch_keys[pitch])) {
                for (long i = perivous_simple; i <= current_simple; i++) {
                    if ((timeline[i] >> pitch * 2) & NOTE_SUSTAIN) {
                        timeline[i] = timeline[i] & ~(NOTE_SUSTAIN << pitch * 2);
                        add_score += SUSTAIN_BASIC_SCORE;
                        isNoise = false;
                    }
                }
            }

            // ノーツないのにタッチしたらノイズにカウント
            if (isNoise) {
                noise_count++;
            }
        }
        score += add_score * pow(NOISE_PENALTY_SCALE, noise_count);
    }

    // 最大コンボ記録
    if (combo > max_combo) max_combo = combo;

    // 音楽開始
    if (start_margin_time <= 0 && current_time == 0 && !isEnd && checksound(audio_handle) == 0) {
        playsound(audio_handle, 0);
    }

    clearCanvas(&canvas);
    
    // ビーバー背景の表示
    float fever_bg_a = (combo / 20.0f > 1.0f)?0.4f: combo / 20.0f * 0.4f;
    drawCanvasToCanvas(&canvas, &fever_bg_canvas[fever_bg_frame_index], 0, 0, 0.0f, fever_bg_a, false, true);

    // 枠のの表示
    drawCanvasToCanvas(&canvas, &borders, 0, 0, 1.0f, 0.0f, false, true);

    if (!isEnd) {

        // 画面に写る最小サンプル
        int min_simple = current_simple - simple_per_pixel * VERTICAL_LINE_X;

        for (int pitch = 0; pitch < TRACK_COUNT; pitch++) {
            int y = pitch * TRACK_HEIGHT + pitch + 1;

            if (!isEnd && isInputHold(pitch_keys[pitch])) {

                // 鳴らしてるエフェクトの表示
                if (isRinging) {
                    COLOR color = pitch_effect_colors[pitch];
                    for (int offset_x = 0; offset_x < SCREEN_WIDTH; offset_x++) {
                        int x = VERTICAL_LINE_X + offset_x;
                        float a = (1.0f - (float)offset_x / (SCREEN_WIDTH - 1.0f)) * 0.4f;
                        drawColorToCanvas(&canvas, x, y - 1, 1, 3, COLOR_WHITE, color, 0.0f, a);
                        drawColorToCanvas(&canvas, x, y, 1, 3, COLOR_WHITE, color, 0.0f, a);
                    }
                }

                // 抑えてるマークの表示
                drawBlockToCanvas(&canvas, VERTICAL_LINE_X, y, pressed_mark, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
            }

            // ノーツの表示
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                int start = limitedSampleIndex(min_simple + simple_per_pixel * x);
                int end = limitedSampleIndex(min_simple + simple_per_pixel * (x + 1));
                COLOR color = pitch_notes_colors[pitch];

                // １ピクセルに内包してるサンプルのループ
                for (long i = start; i < end; i++) {

                    // 小節線
                    if ((timeline[i] & BAR_START) == BAR_START) {
                        drawStringToCanvas(&canvas, x, y, "｜", BORDER_COLOR, color, 1.0f, 0.0f);
                        drawStringToCanvas(&canvas, x, y + 1, "｜", BORDER_COLOR, color, 1.0f, 0.0f);
                        if (pitch < TRACK_COUNT - 1) {
                            drawStringToCanvas(&canvas, x, y + 2, "｜", BORDER_COLOR, color, 1.0f, 0.0f);
                        }
                    }

                    if (((timeline[i] >> pitch * 2) & NOTE_START) == NOTE_START) {
                        // ノーツ頭
                        drawBlockToCanvas(&canvas, x, y, pressed_mark, COLOR_BLACK, color, 1.0f, 1.0f);
                        break;
                    }
                    else if (((timeline[i] >> pitch * 2) & NOTE_SUSTAIN) == NOTE_SUSTAIN) {
                        // サスティン
                        drawColorToCanvas(&canvas, x, y, 1, 2, COLOR_WHITE, color, 1.0f, 1.0f);
                    }
                }
            }

            // トラックのキーの表示
            drawStringToCanvas(&canvas, 0, y + 1, pitch_labels[pitch], BORDER_COLOR, COLOR_BLACK, 1.0f, 0.0f);

            // アタックエフェクトの表示
            ATTACK_QUALITY attack_quality = pitch_attack_quality[pitch].attack_quality;
            if (current_time - pitch_attack_quality[pitch].time < ATTACK_DURATION && attack_quality != ATTACK_QUALITY_NONE) {
                drawStringToCanvas(&canvas, 0, y, quality_labels[attack_quality], getQualityLabelColor(attack_quality), COLOR_BLACK, 1.0f, 0.0f);

                if (attack_quality != ATTACK_QUALITY_BAD) {
                    float o_y = (float)y + 0.5f;
                    float o_x = VERTICAL_LINE_X;
                    float r = (float)(current_time - pitch_attack_quality[pitch].time) / ATTACK_DURATION * SCREEN_WIDTH * 2;
                    COLOR color = pitch_effect_colors[pitch];
                    for (int y = roundl(o_y - r); y < roundl(o_y + r); y++) {
                        for (int x = roundl(o_x - r); x < roundl(o_x + r); x++) {
                            float distance = fabsf(sqrtf(powf(x - o_x, 2) + powf(y - o_y, 2)));
                            float a = distance / r * 0.2f - 0.1f;
                            if (distance < r && a > 0.0f) {
                                drawColorToCanvas(&canvas, x, y, 1, 1, COLOR_WHITE, color, 0.0f, a);
                            }
                        }
                    }
                }
            }
        }
    }

    // コンボの表示
    if (combo > 0) {
        int y_offset = 0;
        int x_offset = 0;
        if (combo_animation_progress < 1.0f) {
            y_offset = -1;
            x_offset = 1;
        }
        drawCanvasToCanvas(&canvas, &combo_canvas, 0 + x_offset, SCREEN_HEIGHT - combo_canvas.height + y_offset, 1.0f, 1.0f, true, true);
        
        sprintf(str_buffer, "%d", combo);
        int i = 0;
        while (str_buffer[i] != '\0') {
            char c;
            int digit;
            sscanf(str_buffer + i, "%1c", &c);
            digit = c - '0';
            drawCanvasToCanvas(&canvas, &number_canvas[digit], (combo_canvas.width + 1) + (number_canvas[digit].width + 1) * i + x_offset, SCREEN_HEIGHT - number_canvas[digit].height + y_offset, 1.0f, 1.0f, true, true);
            i++;
        }
    }

    // スコアの表示
    sprintf_s(str_buffer, "SCORE %d", score);
    drawStringToCanvas(&canvas, 0, SCREEN_HEIGHT - 6, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);

    // 操作ヒントの表示
    if (!isEnd) {
        drawStringToCanvas(&canvas, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 2, hints, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
        drawColorToCanvas(&canvas, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK, COLOR_BLACK, retire_progress, retire_progress);
        drawStringToCanvas(&canvas, SCREEN_WIDTH - 10, SCREEN_HEIGHT - 1, "リタイヤ：Esc 長押し", COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
    }
    
    // リザルトの表示
    if (isEnd) {

        // 曲を停止してリザルトのBGMを流す
        if (audio_handle != 0 && checksound(audio_handle) != 0) {
            stopsound(audio_handle);
        }
        if (result_bgm_handle != 0 && checksound(result_bgm_handle) == 0) {
            playsound(result_bgm_handle, 0);
        }

        // Enterで曲選択に戻る
        if (isInputDown(INPUT_TYPE_ENTER)) {
            if (result_bgm_handle != 0 && checksound(result_bgm_handle) != 0) {
                stopsound(result_bgm_handle);
            }
            if (audio_handle != 0) {
                if (checksound(audio_handle) != 0) {
                    stopsound(audio_handle);
                }
                closesound(audio_handle);
            }
            audio_handle = 0;
            setvolume(getSEHandle(SE_SUBMIT), 100);
            playsound(getSEHandle(SE_SUBMIT), 0);
            callScene(SCENE_SONG_SELECT);
        }

        int x = RESULT_MARGIN_X + RESULT_PADDING_X;
        int y = RESULT_MARGIN_Y + RESULT_PADDING_Y;

        // リザルトの背景色の表示
        drawColorToCanvas(&canvas, RESULT_MARGIN_X, RESULT_MARGIN_Y, SCREEN_WIDTH - RESULT_MARGIN_X * 2, SCREEN_HEIGHT - RESULT_MARGIN_Y * 2, COLOR_BLACK, COLOR_BLACK, 0.7f, 0.7f);

        // スコアの表示
        sprintf(str_buffer, "スコア　%d", score);
        drawStringToCanvas(&canvas, x, y, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);

        // ランク分け、SとAはPERFECTとGREATの色を流用
        float percent = (float)score / basic_full_score;
        CANVAS* rank = &rank_canvas[5];
        COLOR color = COLOR_WHITE;
        if (percent >= 1.0f) {
            color = getQualityLabelColor(ATTACK_QUALITY_PERFECT);
            rank = &rank_canvas[0];
        }
        else if (percent >= 0.8f) {
            color = getQualityLabelColor(ATTACK_QUALITY_GREAT);
            rank = &rank_canvas[1];
        }
        else if (percent >= 0.7f) {
            rank = &rank_canvas[2];
        }
        else if (percent >= 0.6f) {
            rank = &rank_canvas[3];
        }
        else if (percent >= 0.5f) {
            rank = &rank_canvas[4];
        }

        // スコアゲージを表示
        int max_block_num = 20;
        int block_num = roundl(percent * 20);
        for (int i = 0; i < block_num && i < 20; i++) {
            drawStringToCanvas(&canvas, x + i, y + 1, "■", color, COLOR_BLACK, 1.0f, 0.0f);
        }
        for (int i = block_num; i < 20; i++) {
            drawStringToCanvas(&canvas, x + i, y + 1, "■", BORDER_COLOR, COLOR_BLACK, 1.0f, 0.0f);
        }

        // ランクの表示
        int rank_x = max_block_num - rank->width;
        int rank_y = y + 3;
        drawCanvasToCanvas(&canvas, rank, rank_x, rank_y, 0.0f, 1.0f, true, true);
        for (int y = 0; y < rank->height; y++) {
            for (int x = 0; x < rank->width; x++) {
                if (!isBlack(rank->pixels[y * rank->width + x].bg_color)) {
                    drawColorToCanvas(&canvas, rank_x + x, rank_y + y, 1, 1, color, color, 1.0f, 1.0f);
                }
            }
        }

        // 最大コンボ数と各クオリティ数の表示
        sprintf(str_buffer, "最大コンボ　%d", max_combo);
        drawStringToCanvas(&canvas, x, y + 3, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
        for (int i = 0; i < ATTACK_QUALITY_NONE; i++) {
            sprintf(str_buffer, "%d", quality_counts[i]);
            drawStringToCanvas(&canvas, x, y + 4 + i, quality_labels[i], getQualityLabelColor((ATTACK_QUALITY)i), COLOR_BLACK, 1.0f, 0.0f);
            drawStringToCanvas(&canvas, x + 4, y + 4 + i, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
        }

        drawStringToCanvas(&canvas, x, y + 11, "戻る：Enter", COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);

    }

    fever_bg_animation_time += getDeltaTime();
    if (fever_bg_animation_time >= BG_FRAME_DURATION) {
        fever_bg_frame_index = ++fever_bg_frame_index % BG_FRAME_COUNT;
        fever_bg_animation_time = fever_bg_animation_time % BG_FRAME_COUNT;
    }
    combo_animation_progress = updateProgress(combo_animation_progress, (float)getDeltaTime(), COMBO_ANIMATION_DURATION);

    if (start_margin_time > 0) {
        start_margin_time -= getDeltaTime();
    }
    else {
        current_time += getDeltaTime();
    }
    
}