#ifndef BOMBER_RENDERER_H
#define BOMBER_RENDERER_H
#include "game.h"
#include "raylib.h"
#define WINDOW_W 800
#define WINDOW_H 800
bool renderer_init(void);
/* title: 시작 화면만 표시. ready>0: 맵과 READY 문구 표시(남은 대기 시간). */
void renderer_draw(const Game *g,bool forecast,bool paused,bool title,float ready);
void renderer_close(void);
#endif
