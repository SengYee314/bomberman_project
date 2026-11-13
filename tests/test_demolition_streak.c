#include "game.h"
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)
static void place(Actor *a,int x,int y){*a=(Actor){.x=(float)x,.y=(float)y,.cx=x,.cy=y,.tx=x,.ty=y,.dir=DIR_DOWN};}
static Game arena(bool extras){
    Game g;game_init(&g,123,extras);
    for(int y=1;y<MAP_H-1;y++)for(int x=1;x<MAP_W-1;x++)g.map[y][x]=TILE_FLOOR;
    for(int i=0;i<MAX_MONSTERS;i++)g.monsters[i].alive=false;
    return g;
}
static void explode(Game *g,int bricks){
    const int x[4]={8,7,6,7},y[4]={5,6,5,4};
    for(int i=0;i<4;i++)g->map[y[i]][x[i]]=i<bricks?TILE_BRICK:TILE_FLOOR;
    g->bombs[0]=(Bomb){7,5,0,true};
    game_tick(g,(Input){DIR_NONE,false});
}

int main(void){
    int checks=0,failures=0;
    Game g=arena(false);CHECK(!g.demolition_enabled);explode(&g,3);
    CHECK(g.score==0&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    g=arena(true);CHECK(g.demolition_enabled);explode(&g,1);
    CHECK(g.demolition_bricks==1&&g.demolition_ticks_left==1800&&g.score==0);
    g=arena(true);explode(&g,2);g.demolition_ticks_left=1;explode(&g,1);
    CHECK(g.demolition_bricks==1&&g.demolition_ticks_left==1800&&g.score==0);
    explode(&g,1);CHECK(g.demolition_bricks==2&&g.demolition_ticks_left==1799&&g.score==0);
    explode(&g,1);CHECK(g.demolition_bricks==0&&g.demolition_ticks_left==0&&g.score==30);
    CHECK(g.demolition_notice==DEMOLITION_NOTICE_TIME&&g.demolition_pending==0);
    for(int i=0;i<100;i++)game_tick(&g,(Input){DIR_NONE,false});
    CHECK(g.score==30&&g.demolition_bricks==0); /* Persistent fire never recounts. */
    g=arena(true);explode(&g,3);CHECK(g.score==30&&g.demolition_bricks==0);
    explode(&g,4);CHECK(g.score==60&&g.demolition_bricks==1&&g.demolition_ticks_left==1800);

    g=arena(true);explode(&g,1);
    for(int tick=1;tick<=1800;tick++){
        game_tick(&g,(Input){DIR_NONE,false});
        CHECK(g.demolition_ticks_left==1800-tick);
    }
    CHECK(g.demolition_bricks==0&&g.score==0);
    explode(&g,1);CHECK(g.demolition_bricks==1&&g.demolition_ticks_left==1800);
    g.demolition_ticks_left=1;explode(&g,1);
    CHECK(g.demolition_bricks==1&&g.demolition_ticks_left==1800&&g.score==0);

    g=arena(true);explode(&g,1);game_hit_player(&g,false); /* Grace protects this hit. */
    CHECK(g.lives==3&&g.demolition_bricks==1&&g.demolition_ticks_left==1800);
    g.grace=0;game_hit_player(&g,false);
    CHECK(g.lives==2&&g.score==-10&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    CHECK(g.demolition_pending==0&&g.demolition_notice==0);
    explode(&g,3);CHECK(g.score==-10&&g.demolition_bricks==0); /* No progress while stunned. */

    /* A winning blast that also hits the player must not grant a bonus. */
    g=arena(true);explode(&g,2);g.grace=0;place(&g.player,7,5);explode(&g,1);
    CHECK(g.lives==2&&g.score==-20&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    g=arena(true);explode(&g,2);place(&g.player,7,5);explode(&g,1);
    CHECK(g.lives==3&&g.score==30&&g.stun==0&&g.demolition_bricks==0);
    g=arena(true);explode(&g,2);g.grace=0;
    g.monsters[0].alive=true;place(&g.monsters[0].body,1,1);explode(&g,1);
    CHECK(g.lives==2&&g.score==-10&&g.demolition_bricks==0);

    /* Chain reaction revisiting one destroyed brick counts it only once. */
    g=arena(true);g.map[5][9]=TILE_BRICK;
    g.bombs[0]=(Bomb){7,5,0,true};g.bombs[1]=(Bomb){8,5,3,true};
    game_tick(&g,(Input){DIR_NONE,false});
    CHECK(!g.bombs[0].active&&!g.bombs[1].active&&g.map[5][9]==TILE_FLOOR);
    CHECK(g.demolition_bricks==1&&g.score==0);

    g=arena(true);explode(&g,1);g.phase=LEVEL_FLASH;g.flash=LEVEL_FLASH_TIME;
    game_tick(&g,(Input){DIR_NONE,false});CHECK(g.demolition_ticks_left==1800&&g.demolition_bricks==1);
    g.phase=GAME_OVER;game_tick(&g,(Input){DIR_NONE,false});CHECK(g.demolition_ticks_left==1800);
    g.phase=ALL_CLEAR;game_tick(&g,(Input){DIR_NONE,false});CHECK(g.demolition_ticks_left==1800);

    g=arena(true);explode(&g,3);explode(&g,1);g.map[11][13]=TILE_EXIT;place(&g.player,13,11);
    game_check_exit(&g);
    CHECK(g.level==2&&g.phase==LEVEL_FLASH&&g.score==30&&g.demolition_enabled);
    CHECK(g.demolition_bricks==0&&g.demolition_ticks_left==0&&g.demolition_pending==0&&g.demolition_notice==0);
    g.phase=PLAYING;g.level=3;place(&g.player,13,11);g.demolition_bricks=2;g.demolition_ticks_left=50;
    game_check_exit(&g);CHECK(g.phase==ALL_CLEAR&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    g=arena(true);explode(&g,1);g.time_left=0.001f;game_tick(&g,(Input){DIR_NONE,false});
    CHECK(g.phase==GAME_OVER&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    game_init(&g,123,true);CHECK(g.demolition_enabled&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    game_init(&g,123,false);CHECK(!g.demolition_enabled&&g.demolition_bricks==0&&g.demolition_ticks_left==0);
    printf("Demolition streak: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
