#include "renderer.h"
#include <stdio.h>
#include <time.h>
#define READY_TIME 2.0f
typedef enum { SCREEN_TITLE, SCREEN_READY, SCREEN_PLAY } Screen;
static Direction read_direction(void){
    if(IsKeyDown(KEY_UP)||IsKeyDown(KEY_W))return DIR_UP;
    if(IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S))return DIR_DOWN;
    if(IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A))return DIR_LEFT;
    if(IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D))return DIR_RIGHT;
    return DIR_NONE;
}
int main(void){
    InitWindow(WINDOW_W,WINDOW_H,"Bomber Lab - C99 Teaching Kit");SetTargetFPS(60);
    if(!IsWindowReady()){
        fprintf(stderr,"Could not open an OpenGL 3.3 window. Run on a PC with a graphics driver (not remote desktop/VM) and update the GPU driver.\n");
        return 1;
    }
    if(!renderer_init()){
        fprintf(stderr,"assets/spritesheet.png missing; start from project/build directory.\n");
        CloseWindow();return 1;
    }
    bool extras = false; 
    Game g;game_init(&g,(uint32_t)time(NULL),extras);
    Screen screen=SCREEN_TITLE;
    bool paused=false,forecast=true,pending_bomb=false;
    float accumulator=0,ready=0;
    while(!WindowShouldClose()){
        float dt=GetFrameTime();if(dt>.1f)dt=.1f;
        if(screen==SCREEN_TITLE){
            /* 아무 키나 누르면 시작 화면에서 게임 화면(READY 대기)으로 이동한다. */
            int key=GetKeyPressed();
            if(key!=0){
                if(key==KEY_F2||key==KEY_F3){
                    extras=key==KEY_F3;game_init(&g,(uint32_t)time(NULL),extras);
                }
                screen=SCREEN_READY;ready=READY_TIME;
            }
        }else{
            if(IsKeyPressed(KEY_F))forecast=!forecast;
            if(IsKeyPressed(KEY_F2)||IsKeyPressed(KEY_F3)){
                extras=IsKeyPressed(KEY_F3);game_init(&g,(uint32_t)time(NULL),extras);screen=SCREEN_READY;ready=READY_TIME;paused=false;
            }
            if(IsKeyPressed(KEY_R)){game_init(&g,(uint32_t)time(NULL),extras);screen=SCREEN_READY;ready=READY_TIME;paused=false;}
            if(screen==SCREEN_READY){
                /* 맵과 READY 문구를 보여 주고 대기 시간이 지나면 게임을 시작한다. 대기 중 입력은 무시한다. */
                ready-=dt;if(ready<=0){ready=0;screen=SCREEN_PLAY;accumulator=0;pending_bomb=false;}
            }else{
                if(IsKeyPressed(KEY_P)){paused=!paused;pending_bomb=false;}
                /* bm_stage_start가 valid=false면 시작 상태가 없으므로 진행하지 않고 renderer가 구현 안내만 표시한다. */
                if(!paused&&!g.stage_invalid){
                    pending_bomb=pending_bomb||IsKeyPressed(KEY_SPACE);
                    accumulator+=dt;
                    while(accumulator>=FIXED_DT&&!g.stage_invalid){game_tick(&g,(Input){read_direction(),pending_bomb});pending_bomb=false;accumulator-=FIXED_DT;}
                }else{accumulator=0;pending_bomb=false;}
            }
        }
        BeginDrawing();renderer_draw(&g,forecast,paused,screen==SCREEN_TITLE,screen==SCREEN_READY?ready:0);EndDrawing();
        if(IsKeyPressed(KEY_F12))TakeScreenshot("bomber_screenshot.png");
    }
    renderer_close();CloseWindow();return 0;
}
