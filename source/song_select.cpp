
#include <stdlib.h>
#include "song_select.h"
#include "canvas.h"
#include "main.h"
#include "song.h"
#include "game.h"
#include "input_center.h"

#define CONIOEX
#include "conioex.h"

#define SONG_CARD_WIDTH 16
#define SONG_CARD_HEIGHT 5
#define SONG_CARD_X 1
#define ARROW_X 5
#define SCROLL_TRANSITATION_DURATION 300
#define CONTROL_LABEL "確定：Enter 　戻る：Esc"
#define DEFAULT_SOUND_DELAY_CORRECTION 250
#define MAX_SOUND_DELAY_CORRECTION 1000
#define MIN_SOUND_DELAY_CORRECTION 0
#define SOUND_DELAY_CORRECTION_CHANGE_DURATION 30

const float PI = 4.0f * atan(1);

enum SCROLL_DIRECT {
	SCROLL_DIRECT_UP,
	SCROLL_DIRECT_DOWN
};

typedef struct _SONG_CARD {
	SONG song = { "", "" , "" , "" , "" , 0 };
	CANVAS canvas = { NULL, 0, 0 };
	CANVAS cover = { NULL, 0, 0 };
} SONG_CARD;

static const SCENE THIS_SCENE = SCENE_SONG_SELECT;
static CANVAS canvas = { NULL, 0, 0 };
static const int COVER_X = SCREEN_WIDTH - 20 - 1;
static const int COVER_Y = 1;
static const float scroll_speeds[] = {
	0.5f,
	0.8f,
	1.0f,
	1.2f,
	1.5f,
	2.0f
};

static int song_count;
static SONG_CARD *song_cards;
static int current_song_index = 0;
static int scroll_speed_index = 2;
static CANVAS* previous_cover = NULL;

static SCROLL_DIRECT scroll_direct = SCROLL_DIRECT_DOWN;
static float scroll_transitaion_progress = 1.0f;
static int sound_delay_correction = DEFAULT_SOUND_DELAY_CORRECTION;
static float sound_delay_correction_change_progress = 1.0f;
static int audio_handle;
struct BAND
{
	short low;
	short high;
};
static BAND preview_bands[] = {
	{50, 80}
};
static float preview_band_mass[_countof(preview_bands)];
static char str_buffer[256];

void songSelectInit() {
	// シーンキャンパス作成＆登録
	createCanvas(SCREEN_WIDTH, SCREEN_HEIGHT, &canvas);
	setSceneCanvas(THIS_SCENE, &canvas);

	SONG* song_list = getSongList();
	song_count = getSongCount();

	// 曲カード作成
	song_cards = (SONG_CARD*)malloc(sizeof(SONG_CARD) * song_count);
	if (song_cards != NULL) {
		memset(song_cards, 0, sizeof(SONG_CARD) * song_count);
		float s = 0.3f;
		for (int i = 0; i < song_count; i++) {
			song_cards[i].song = song_list[i];
			createCanvas(SONG_CARD_WIDTH, SONG_CARD_HEIGHT, &song_cards[i].canvas);

			// 曲名、アーティスト、BPMを書き込む
			drawStringToCanvas(&song_cards[i].canvas, 1, 1, song_cards[i].song.name, COLOR_WHITE, COLOR_BLACK, 1.0f, 1.0f);
			drawStringToCanvas(&song_cards[i].canvas, 1, 2, song_cards[i].song.artist, COLOR_WHITE, COLOR_BLACK, 1.0f, 1.0f);
			sprintf(str_buffer, "BPM %d", song_cards[i].song.bpm);
			drawStringToCanvas(&song_cards[i].canvas, 1, 3, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 1.0f);

			// カードの色設定
			float h = 1.0f / 6.0f * (i % 6);
			for (int y = 0; y < song_cards[i].canvas.height; y++) {
				for (int x = 0; x < song_cards[i].canvas.width; x++) {
					int index = y * song_cards[i].canvas.width + x;
					float v = (1.0f - ((float)x / (song_cards[i].canvas.width - 1))) * 0.5f + (1.0f - ((float)y / (song_cards[i].canvas.height - 1))) * 0.5f - 0.2f;
					if (v < 0.0f) {
						v = 0.0f;
					}
					song_cards[i].canvas.pixels[index].bg_color = hsv2rgb(h, s, v);
				}
			}

			// ジャケット画像読み込む
			createCanvasByBmp(song_cards[i].song.cover_file, &song_cards[i].cover);
		}
		
	}
	else {
		song_count = 0;
	}

	if (song_cards != NULL) {
		audio_handle = opensound(song_cards[current_song_index].song.audio_file);
	}
	previous_cover = NULL;
	
}

void songSelectDestory() {
	// シーンキャンパス削除
	destoryCanvas(&canvas);
	setSceneCanvas(THIS_SCENE, NULL);

	if (song_cards != NULL) {
		for (int i = 0; i < song_count; i++) {
			destoryCanvas(&song_cards[i].canvas);
			destoryCanvas(&song_cards[i].cover);
		}
		free(song_cards);
		song_cards = NULL;
	}
	previous_cover = NULL;

	if (audio_handle != 0) {
		closesound(audio_handle);
	}
	audio_handle = 0;
}

void songSelectScene() {

	if (!inSceneTransition()) {

		// 曲変更の処理、スクロール中は処理しない
		if (scroll_transitaion_progress >= 1.0f) {
			if (isInputHold(INPUT_TYPE_UP) || isInputHold(INPUT_TYPE_DOWN)) {
				playsound(getSEHandle(SE_CHANGE), 0);
				previous_cover = &(song_cards[current_song_index].cover);
				scroll_transitaion_progress = 0.0f;

				if (isInputHold(INPUT_TYPE_UP)) {
					current_song_index = previousIndex(current_song_index, song_count);
					scroll_direct = SCROLL_DIRECT_UP;
				}
				else if (isInputHold(INPUT_TYPE_DOWN)) {
					current_song_index = nextIndex(current_song_index, song_count);
					scroll_direct = SCROLL_DIRECT_DOWN;
				}

				// プレビュー曲更新
				if (audio_handle != 0) {
					closesound(audio_handle);
				}
				audio_handle = opensound(song_cards[current_song_index].song.audio_file);
			}
		}


		// 譜面スクロール速度選択
		if (isInputDown(INPUT_TYPE_LEFT)) {
			playsound(getSEHandle(SE_CHANGE), 0);
			scroll_speed_index = nextIndex(scroll_speed_index, _countof(scroll_speeds));
		}
		else if (isInputDown(INPUT_TYPE_RIGHT)) {
			playsound(getSEHandle(SE_CHANGE), 0);
			scroll_speed_index = previousIndex(scroll_speed_index, _countof(scroll_speeds));
		}

		// 音遅延補正選択
		if (sound_delay_correction_change_progress >= 1.0f) {
			if (isInputHold(INPUT_TYPE_F1) && sound_delay_correction < MAX_SOUND_DELAY_CORRECTION) {
				sound_delay_correction++;
				sound_delay_correction_change_progress = 0.0f;
			}
			else if (isInputHold(INPUT_TYPE_F2) && sound_delay_correction > MIN_SOUND_DELAY_CORRECTION) {
				sound_delay_correction--;
				sound_delay_correction_change_progress = 0.0f;
			}
		}

		// 確定ボタン
		if (isInputDown(INPUT_TYPE_ENTER)) {
			playsound(getSEHandle(SE_SUBMIT), 0);

			// シーン遷移完了するまでシーンの後処理行われないから
			// プレビューと本番が同一オーディオファイルの場合
			// ファイル展開できないので
			// 前もって曲プレビューのハンドルを閉じる
			if (audio_handle != 0) {
				closesound(audio_handle);
			}
			audio_handle = 0;

			// 譜面速度と選択した曲をセット
			gameSetSoundDelay(sound_delay_correction);
			gameSetScrollSpeed(scroll_speeds[scroll_speed_index]);
			gameSetSong(song_cards[current_song_index].song);

			callScene(SCENE_GAME);
		}
		else if (isInputDown(INPUT_TYPE_ESC)) {
			// タイトルに戻る
			playsound(getSEHandle(SE_CANCEL), 0);
			callScene(SCENE_TITLE);
		}
	}

	// プレビュー曲流す
	if (audio_handle != 0 && checksound(audio_handle) == 0) {
		playsound(audio_handle, 0);
	}
	
	// 曲カード位置更新
	int prevous_song_index = previousIndex(current_song_index, song_count);
	int next_song_index = nextIndex(current_song_index, song_count);
	float margin = SCREEN_HEIGHT / 2.0f - SONG_CARD_HEIGHT / 2.0f;
	float current_y = margin;
	float prevous_y = 0;
	float next_y = current_y * 2;
	float scroll_offset = (scroll_direct == SCROLL_DIRECT_UP)?-current_y: current_y;

	prevous_y = prevous_y * scroll_transitaion_progress + (prevous_y + scroll_offset) * (1.0f - scroll_transitaion_progress);
	next_y = next_y * scroll_transitaion_progress + (next_y + scroll_offset) * (1.0f - scroll_transitaion_progress);
	current_y = current_y * scroll_transitaion_progress + (current_y + scroll_offset) * (1.0f - scroll_transitaion_progress);

	// 上、中、下のカードの透明度
	float p_a = 1.0f - (fabsf(prevous_y - margin) / margin) * 0.8f;
	float c_a = 1.0f - (fabsf(current_y - margin) / margin) * 0.8f;
	float n_a = 1.0f - (fabsf(next_y - margin) / margin) * 0.8f;
	if (p_a < 0.0f) p_a = 0.0f;
	if (c_a < 0.0f) c_a = 0.0f;
	if (n_a < 0.0f) n_a = 0.0f;


	// 表示部分

	clearCanvas(&canvas);

	// 曲変更時のジャケット透過効果のため二枚重ね
	if (previous_cover != NULL) {
		drawCanvasToCanvas(&canvas, previous_cover, COVER_X, COVER_Y, 1.0f, 1.0f, true, true);
	}
	drawCanvasToCanvas(&canvas, &song_cards[current_song_index].cover, COVER_X, COVER_Y, scroll_transitaion_progress, scroll_transitaion_progress, true, true);

	// 上、中、下のカードの表示
	drawCanvasToCanvas(&canvas, &song_cards[prevous_song_index].canvas, SONG_CARD_X, roundl(prevous_y), p_a, p_a, false, true);
	drawCanvasToCanvas(&canvas, &song_cards[next_song_index].canvas, SONG_CARD_X, roundl(next_y), n_a, n_a, false, true);
	drawCanvasToCanvas(&canvas, &song_cards[current_song_index].canvas, SONG_CARD_X, roundl(current_y), c_a, c_a, false, true);

	// 上下矢印の表示
	drawStringToCanvas(&canvas, ARROW_X, margin - SONG_CARD_HEIGHT / 2, "▲", COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
	drawStringToCanvas(&canvas, ARROW_X, margin + SONG_CARD_HEIGHT + 1, "▼", COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);

	// 操作ヒントの表示
	sprintf(str_buffer, "音遅延補正%4dms（F1：増加　F2：減少）", sound_delay_correction);
	drawStringToCanvas(&canvas, SCREEN_WIDTH - 1 - strlen(str_buffer) / 2, SCREEN_HEIGHT - 4, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
	sprintf(str_buffer, "譜面スピード　←　%1.1fx　→", scroll_speeds[scroll_speed_index]);
	drawStringToCanvas(&canvas, SCREEN_WIDTH - 1 - strlen(str_buffer) / 2, SCREEN_HEIGHT - 3, str_buffer, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);
	drawStringToCanvas(&canvas, SCREEN_WIDTH - 2 - strlen(CONTROL_LABEL) / 2, SCREEN_HEIGHT - 2, CONTROL_LABEL, COLOR_WHITE, COLOR_BLACK, 1.0f, 0.0f);

	scroll_transitaion_progress = updateProgress(scroll_transitaion_progress, (float)getDeltaTime(), SCROLL_TRANSITATION_DURATION);
	sound_delay_correction_change_progress = updateProgress(sound_delay_correction_change_progress, (float)getDeltaTime(), SOUND_DELAY_CORRECTION_CHANGE_DURATION);
}

/*
* 
　　没機能

	TODO:
	// Fourier transform

	circle = 4
	k = [sample per sec] / [Hz]
	n = k * circle
	re
	im
	for(i=0;i < n;i++){
		t = (i%k) / k;
		re += cos(-2 * PI * t)
		im += sin(-2 * PI * t)
	}
	re /= n
	im /= n
	result = abs(sqrt(pow(re, 2) + pow(im, 2)))

	void loadPreview() {
	if (preview_file == NULL || preview_data_ready)
		return;
	if (audio_handle != 0) {
		closesound(audio_handle);
	}
	audio_handle = opensound(preview_file);
	if (audio_handle == 0)
		return;

	FILE* fp;
	fp = fopen(preview_file, "rb");
	if (fp == NULL)
		return;

	char header_ID[4];
	long header_size;
	char header_type[4];
	char fmt_ID[4];
	long fmt_size;
	short fmt_format;
	long fmt_bytes_per_sec;
	short fmt_block_size;
	char data_ID[4];
	long data_size;

	fread(header_ID, 1, 4, fp);
	fread(&header_size, 4, 1, fp);
	fread(header_type, 1, 4, fp);
	fread(fmt_ID, 1, 4, fp);
	fread(&fmt_size, 4, 1, fp);
	fread(&fmt_format, 2, 1, fp);
	fread(&preview_channel, 2, 1, fp);
	fread(&preview_samples_per_sec, 4, 1, fp);
	fread(&fmt_bytes_per_sec, 4, 1, fp);
	fread(&fmt_block_size, 2, 1, fp);
	fread(&preview_bits_per_sample, 2, 1, fp);
	fread(data_ID, 1, 4, fp);
	fread(&data_size, 4, 1, fp);

	if (preview_data != NULL) {
		free(preview_data);
		preview_data = NULL;
	}

	preview_data = (short*)malloc(data_size);
	if (preview_data != NULL) {
		fread(preview_data, data_size, 1, fp);
		preview_current_sample = preview_data;
		memset(preview_band_mass, 0, _countof(preview_bands));
		preview_data_ready = true;
	}
	fclose(fp);

}
*/
