#include "renderer.h"
#include "rules.h"
#include <math.h>
#include <stdio.h>
static Texture2D atlas,hud;
static const int TILE=48,OX=40,OY=116;
static const Color BG={18,21,18,255},GREEN_FIELD={15,137,36,255};
/* 모든 셀은 64×64. assets/manifest.json의 주소와 일치 */
static void sprite(int frame,float x,float y,float w,float h,Color tint){
    Rectangle source={(float)((frame%8)*64),(float)((frame/8)*64),64,64};
    DrawTexturePro(atlas,source,(Rectangle){x,y,w,h},(Vector2){0,0},0,tint);
}
static void centered(const char *s,int y,int size,Color c){DrawText(s,(WINDOW_W-MeasureText(s,size))/2,y,size,c);}
static void tile_sprite(int f,int x,int y,Color tint){sprite(f,(float)(OX+x*TILE),(float)(OY+y*TILE),TILE,TILE,tint);}
static void actor_sprite(int f,const Actor *a,Color tint){sprite(f,OX+(a->x-.5f)*TILE+TILE/2,OY+(a->y-.5f)*TILE+TILE/2,TILE,TILE,tint);}
bool renderer_init(void){
    const char *paths[]={"assets/spritesheet.png","../assets/spritesheet.png","../../assets/spritesheet.png"};
    for(int i=0;i<3;i++)if(FileExists(paths[i])){atlas=LoadTexture(paths[i]);break;}
    if(atlas.id==0)return false;
    const char *hudpaths[]={"assets/hud_panel.png","../assets/hud_panel.png","../../assets/hud_panel.png"};
    for(int i=0;i<3;i++)if(FileExists(hudpaths[i])){hud=LoadTexture(hudpaths[i]);break;}
    SetTextureFilter(atlas,TEXTURE_FILTER_POINT);if(hud.id)SetTextureFilter(hud,TEXTURE_FILTER_POINT);return true;
}
void renderer_close(void){if(atlas.id)UnloadTexture(atlas);if(hud.id)UnloadTexture(hud);}
static void draw_help(void){
    DrawText("ARROWS / WASD MOVE    SPACE BOMB    P PAUSE    R RESTART",40,751,14,(Color){218,224,210,255});
    DrawText("F2 CORE   F3 EXTRAS   F12 SCREENSHOT",40,774,12,(Color){137,157,127,255});
}
/* 시작 화면: 게임 화면과 분리된 별도 화면. 아무 키나 누르면 main.c가 READY로 넘긴다. */
static void draw_title(void){
    DrawRectangle(OX-4,40,MAP_W*TILE+8,OY-40+MAP_H*TILE+4,(Color){5,24,7,255});
    centered("BOMBER LAB",240,64,(Color){255,216,77,255});
    const int frames[5]={0,16,20,24,37};
    for(int i=0;i<5;i++)sprite(frames[i],(float)(WINDOW_W/2-5*64/2+i*64+4),330,56,56,WHITE);
    if((int)(GetTime()*2)%2==0)centered("PRESS ANY KEY TO START",430,24,WHITE);
    centered("CLEAR THREE STAGES THROUGH THE EXIT",480,16,LIGHTGRAY);
    centered("F2 CORE / F3 EXTRAS: PORTALS + DEMOLITION",530,16,(Color){140,230,200,255});
    centered("DESTROY 3 BRICKS IN 15s WITHOUT DAMAGE: +30",560,14,LIGHTGRAY);
}
void renderer_draw(const Game *g,bool forecast,bool paused,bool title,float ready){
    ClearBackground(BG);
    DrawText("BOMBER LAB",25,12,18,(Color){208,222,194,255});
    DrawText(g->portals_enabled?"EXTRAS MODE":"CORE MODE",650,14,14,(Color){199,216,174,255});
    if(g->demolition_enabled&&!title){
        if(g->demolition_notice>0){
            DrawText(TextFormat("+30 BONUS!  NEXT %d/3",g->demolition_bricks),230,14,14,YELLOW);
        }else if(g->demolition_ticks_left>0){
            float seconds=ceilf(g->demolition_ticks_left*FIXED_DT*10)/10;
            DrawText(TextFormat("DEMOLITION %d/3   %.1fs",g->demolition_bricks,seconds),230,14,14,WHITE);
        }else{
            DrawText("DEMOLITION 0/3  |  15s / +30",230,14,14,(Color){140,230,200,255});
        }
        DrawRectangle(230,32,350,3,(Color){47,56,44,255});
        DrawRectangle(230,32,350*g->demolition_ticks_left/DEMOLITION_WINDOW_TICKS,3,(Color){140,230,200,255});
    }
    if(title){draw_title();draw_help();return;}
    DrawRectangle(24,40,752,64,(Color){184,186,181,255});
    DrawRectangle(24,101,752,3,(Color){84,88,81,255});
    if(hud.id)DrawTexturePro(hud,(Rectangle){0,0,752,64},(Rectangle){24,40,752,64},(Vector2){0,0},0,WHITE);
    DrawText("LEVEL",40,49,14,DARKGRAY);DrawText(TextFormat("%02i",g->level),40,69,25,BLACK);
    DrawText("TIME",190,49,14,DARKGRAY);DrawText(TextFormat("%03i",(int)ceilf(g->time_left)),190,69,25,g->time_left<30?MAROON:BLACK);
    DrawText("SCORE",360,49,14,DARKGRAY);DrawText(TextFormat("%06i",g->score),360,69,25,BLACK);
    DrawText("LIVES",590,49,14,DARKGRAY);
    for(int i=0;i<START_LIVES;i++)sprite(i<g->lives?52:63,590+i*37,65,32,32,WHITE);
    DrawRectangle(OX-4,OY-4,MAP_W*TILE+8,MAP_H*TILE+8,(Color){5,24,7,255});
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
        DrawRectangle(OX+x*TILE,OY+y*TILE,TILE,TILE,GREEN_FIELD);
        tile_sprite(32,x,y,WHITE);
        Tile t=g->map[y][x];
        if(t==TILE_WALL)tile_sprite(33,x,y,WHITE);
        if(t==TILE_BRICK)tile_sprite(34,x,y,WHITE);
        if(t==TILE_EXIT)tile_sprite(36,x,y,WHITE);
        if(t==TILE_PORTAL_A||t==TILE_PORTAL_B){
            tile_sprite(54+((int)(GetTime()*5)%2),x,y,t==TILE_PORTAL_A?SKYBLUE:VIOLET);
            DrawRectangle(OX+x*TILE+3,OY+y*TILE+3,15,18,(Color){4,12,5,220});
            DrawText(t==TILE_PORTAL_A?"A":"B",OX+x*TILE+5,OY+y*TILE+4,14,WHITE);
        }
    }
    if(forecast&&g->forecast_enabled){
        bool danger[MAP_H][MAP_W];int path[MAP_W*MAP_H][2];int length=game_forecast(g,danger,path);
        for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)if(danger[y][x])DrawRectangle(OX+x*TILE+3,OY+y*TILE+3,TILE-6,TILE-6,(Color){255,74,30,72});
        for(int i=1;i<length;i++)DrawLineEx((Vector2){OX+path[i-1][0]*TILE+24,OY+path[i-1][1]*TILE+24},(Vector2){OX+path[i][0]*TILE+24,OY+path[i][1]*TILE+24},3,(Color){63,240,236,240});
        if(length)DrawCircle(OX+path[length-1][0]*TILE+24,OY+path[length-1][1]*TILE+24,7,(Color){63,240,236,240});
    }
    for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active){const Bomb *b=&g->bombs[i];int f=37+(int)((BOMB_FUSE-b->fuse)*4)%3;tile_sprite(f,b->x,b->y,WHITE);}
    for(int i=0;i<MAX_MONSTERS;i++)if(g->monsters[i].alive){const Monster *m=&g->monsters[i];int base=m->kind==MONSTER_WHITE?16:m->kind==MONSTER_YELLOW?20:24;actor_sprite(base+(int)m->body.anim%4,&m->body,WHITE);}
    int pf=g->player.dir==DIR_DOWN?0:g->player.dir==DIR_RIGHT?4:g->player.dir==DIR_UP?8:12;
    if(g->stun>0)pf=28+(int)(GetTime()*5)%4;
    else if(g->player.moving)pf+=(int)g->player.anim%4;
    if(g->grace<=0||(int)(GetTime()*12)%2==0)actor_sprite(pf,&g->player,WHITE);
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
        if(g->crumble[y][x]>0){int f=(int)((.4f-g->crumble[y][x])*10);if(f>3)f=3;if(f<0)f=0;tile_sprite(48+f,x,y,WHITE);}
        if(g->flame[y][x]>0){
            bool u=y>0&&g->flame[y-1][x]>0,d=y+1<MAP_H&&g->flame[y+1][x]>0,l=x>0&&g->flame[y][x-1]>0,r=x+1<MAP_W&&g->flame[y][x+1]>0;
            int f=(!u&&!d&&!l&&!r)?40:(u||d)&&(l||r)?40+(int)(GetTime()*12)%2:(u||d)?(!u?44:!d?46:43):(!l?47:!r?45:42);
            tile_sprite(f,x,y,WHITE);
        }
    }
    /* 흰색 점멸 여부는  bm_level_flash에 맡겨짐. dt=0은 현재 상태 조회. */
    if(g->phase==LEVEL_FLASH&&!g->stage_invalid&&bm_level_flash(g->flash,0).white)DrawRectangle(OX,OY,MAP_W*TILE,MAP_H*TILE,(Color){255,255,255,220});
    draw_help();
    DrawRectangle(587,774,173,8,(Color){47,56,44,255});
    DrawRectangle(587,774,(int)(173*(1-g->cooldown/BOMB_COOLDOWN)),8,(Color){214,237,119,255});
    if(g->stun>0)DrawText(TextFormat("REVIVE IN %.1f",g->stun),534,744,16,YELLOW);
    if(g->stage_invalid){
        /* bm_stage_start 미구현 안내.*/
        DrawRectangle(OX,OY,MAP_W*TILE,MAP_H*TILE,(Color){4,12,5,220});
        centered("IMPLEMENT bm_stage_start FIRST",330,30,(Color){255,216,77,255});
        centered("bm_stage_start() returned valid=false,",385,18,WHITE);
        centered("so the stage has no start state and cannot run.",410,18,WHITE);
        centered("Fill in TODO STAGE_START in src/rules.c",455,18,(Color){140,230,140,255});
        centered("(see the function comments in src/rules.h)",482,16,LIGHTGRAY);
        centered("Rebuild and run again.",520,16,LIGHTGRAY);
    }
    else if(ready>0){
        /* 시작 대기: 맵이 보이도록 가운데에만 READY 문구를 표시한다. */
        DrawRectangle(OX,OY+MAP_H*TILE/2-44,MAP_W*TILE,88,(Color){4,12,5,200});
        centered("READY",OY+MAP_H*TILE/2-34,52,(Color){255,216,77,255});
        centered(TextFormat("LEVEL %02i",g->level),OY+MAP_H*TILE/2+20,18,WHITE);
    }
    else if(g->phase==GAME_OVER||g->phase==ALL_CLEAR||paused){
        DrawRectangle(OX,OY,MAP_W*TILE,MAP_H*TILE,(Color){4,12,5,192});
        if(g->phase==GAME_OVER){centered("GAME OVER",350,52,RED);centered("PRESS R TO RESTART",422,20,WHITE);}
        else if(g->phase==ALL_CLEAR){centered("ALL CLEAR!",350,52,GOLD);centered(TextFormat("FINAL SCORE  %i",g->score),417,24,WHITE);centered("PRESS R TO RESTART",460,16,LIGHTGRAY);}
        else centered("PAUSED",375,48,WHITE);
    }
}
