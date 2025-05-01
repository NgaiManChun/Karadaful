#ifndef _SONG_H
#define _SONG_H

#define TRACK_COUNT 6
#define SIMPLING_RATE 60

#define NOTE_START 0x02
#define NOTE_SUSTAIN 0x01
#define BAR_START (0x01 << (sizeof(unsigned short) * 8 - 1))

typedef struct _SONG {
	char name[64];
	char artist[64];
	char audio_file[256];
	char cover_file[256];
	char tempo_groups_file[256];
	int length;
	int bpm;
	float basic_view_second;
} SONG;

SONG* getSongList();
void loadSongList();
void destorySongList();
int getSongCount();
void loadTempoGroups(const char* file, unsigned short* timeline, int song_length);

#endif