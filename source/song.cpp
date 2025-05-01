
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "song.h"

#define CONIOEX
#include "conioex.h"

#define SONG_LIST_FILE "asset\\song_list.csv"
#define ROW_LENGTH (64 * 2 + 256 * 3)

static SONG *song_list;
static int song_count = 0;


int csvLineCount(const char* file) {
	int count = 0;
	FILE* fp;
	fp = fopen(file, "r");
	if (fp != NULL) {
		char buf[256];
		int read_size;
		while ((read_size = fread(buf, 1, 256, fp)) > 0) {
			for (int i = 0; i < read_size; i++) {
				if (buf[i] == '\n') count++;
			}
		}
		fclose(fp);
	}
	return count;
}

SONG* getSongList() {
	if (song_list == 0) {
		loadSongList();
	}
	return song_list;
}

// CSVファイルからソングリスト作成
void loadSongList() {
	int item_count = csvLineCount(SONG_LIST_FILE) - 1; // ヘッダー分を減らす
	FILE* fp = fopen(SONG_LIST_FILE, "r");
	if (fp != NULL) {

		char row_buffer[ROW_LENGTH];

		song_list = (SONG *)malloc(sizeof(SONG) * item_count);
		song_count = item_count;

		// ヘッダー
		fgets(row_buffer, ROW_LENGTH, fp);

		// 一行目
		fgets(row_buffer, ROW_LENGTH, fp);

		
		item_count = 0;
		while (!feof(fp)) {
			strcpy_s(song_list[item_count].name, strtok(row_buffer, ","));
			strcpy_s(song_list[item_count].artist, strtok(NULL, ","));
			sscanf_s(strtok(NULL, ","), "%d", &song_list[item_count].bpm);
			sscanf_s(strtok(NULL, ","), "%d", &song_list[item_count].length);
			sscanf_s(strtok(NULL, ","), "%f", &song_list[item_count].basic_view_second);
			strcpy_s(song_list[item_count].audio_file, strtok(NULL, ","));
			strcpy_s(song_list[item_count].cover_file, strtok(NULL, ","));
			strcpy_s(song_list[item_count].tempo_groups_file, strtok(NULL, "\n"));

			item_count++;

			// 次の行読み込む
			fgets(row_buffer, ROW_LENGTH, fp);
		}
		
		fclose(fp);
	}
}

void destorySongList() {
	if (song_list != 0) {
		free(song_list);
		song_list = 0;
		song_count = 0;
	}
}

int getSongCount() {
	return song_count;
}

// テンポグループのノーツをタイムラインに書き込む
void loadTimelineNotes(const char* file, unsigned short* timeline, float second_per_bar, float start_offset, float end_secord, int song_length) {
	FILE* fp;
	fp = fopen(file, "rt");
	if (fp != NULL) {
		char row_buffer[ROW_LENGTH];

		// ヘッダー
		fgets(row_buffer, ROW_LENGTH, fp);

		// 一行目
		fgets(row_buffer, ROW_LENGTH, fp);

		long end_of_timeline = SIMPLING_RATE * song_length;
		long end_of_tempo = SIMPLING_RATE * end_secord;
		int simple_per_bar = SIMPLING_RATE * second_per_bar;

		// 小節線を振り分け
		/*for (long i = SIMPLING_RATE * start_offset; i < end_of_timeline && i < end_of_tempo; i += simple_per_bar) {
			timeline[i] = timeline[i] | BAR_START;
		}*/
		for (int i = 0; i * second_per_bar < end_secord; i++) {
			long index = roundl((start_offset + second_per_bar * i) * SIMPLING_RATE);
			timeline[index] = timeline[index] | BAR_START;
		}

		while (!feof(fp)) {
			int pitch = 0;
			float position;
			float length;
			sscanf_s(row_buffer, "%d,%f,%f", &pitch, &position, &length);
			long start_simple_note = roundl((start_offset + second_per_bar * position) * SIMPLING_RATE);

			// ノーツは時間順じゃない可能性あるのでbreakしないように
			if (start_simple_note >= end_of_timeline)
				continue;

			// サスティンを書き込む
			long sustain_to = start_simple_note + ceil(second_per_bar * length * SIMPLING_RATE);
			for (long i = start_simple_note; i < sustain_to && i < end_of_timeline; i++) {
				timeline[i] = timeline[i] | (NOTE_SUSTAIN << pitch * 2);
			}

			// ノーツ頭を書き込む
			timeline[start_simple_note] = timeline[start_simple_note] | (NOTE_START << pitch * 2);

			fgets(row_buffer, ROW_LENGTH, fp);
		}
		fclose(fp);
	}
}

// CSVファイル（テンポファイル）を読み込む
void loadTempoGroups(const char* file, unsigned short* timeline, int song_length) {
	FILE* fp;
	fp = fopen(file, "rt");
	if (fp != NULL) {
		char row_buffer[ROW_LENGTH];

		// ヘッダー
		fgets(row_buffer, ROW_LENGTH, fp);

		// 一行目
		fgets(row_buffer, ROW_LENGTH, fp);

		// 同じ曲に複数のテンポや
		// 途中の１小節頭だけ２拍しかない
		// などの場合、テンポグループも複数になる
		char notes_file[256];
		while (!feof(fp)) {
			float second_per_bar; // １小節頭の長さ（秒）
			float start_offset; // テンポグループ開始秒数
			float end_secord; // このテンポが終わる秒数
			sscanf_s(strtok(row_buffer, ","), "%f", &second_per_bar);
			sscanf_s(strtok(NULL, ","), "%f", &start_offset);
			sscanf_s(strtok(NULL, ","), "%f", &end_secord);
			strcpy_s(notes_file, strtok(NULL, "\n"));

			loadTimelineNotes(notes_file, timeline, second_per_bar, start_offset, end_secord, song_length);

			fgets(row_buffer, ROW_LENGTH, fp);
		}
		fclose(fp);
	}
}
