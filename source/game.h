#include "song.h"

#ifndef _GAME_H
#define _GAME_H

void gameSetScrollSpeed(float _scroll_speed);
void gameSetSoundDelay(int _sound_delay);
void gameSetSong(SONG _song);
void gameInit();
void gameDestory();
void gameScene();

#endif