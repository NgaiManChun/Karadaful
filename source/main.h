#include "canvas.h"
#include <windows.h>
#ifndef _MAIN_H
#define _MAIN_H

#define FRAME_RATE 60
#define SCREEN_WIDTH 40
#define SCREEN_HEIGHT 25
#define DEFAULT_SCENE_TRANSITATION_DURTAION 500

enum SCENE {
    SCENE_TITLE,
    SCENE_SONG_SELECT,
    SCENE_GAME,
    SCENE_NONE,
    SCENE_COUNT
};

enum SCENE_STATE {
    SCENE_STATE_CONTINUE,
    SCENE_STATE_END
};

enum SCENE_TRANSITATION {
    SCENE_TRANSITATION_NONE,
    SCENE_TRANSITATION_TRANSPARENT,
    SCENE_TRANSITATION_FILL_CIRCLE
};

enum SE {
    SE_CHANGE,
    SE_SUBMIT,
    SE_CANCEL
};

void setSceneCanvas(SCENE scene, CANVAS* canvas);
CANVAS* getSceneCanvas(SCENE scene);
void initScene(SCENE scene);
void destoryScene(SCENE scene);
void callScene(SCENE scene);
bool inSceneTransition();
void setTransition(SCENE_TRANSITATION transitation, int duration);
int getDeltaTime();
int getSEHandle(SE se);
int nextIndex(int index, int limit);
int previousIndex(int index, int limit);
float updateProgress(float current, float deltaTime, float duration);


#endif