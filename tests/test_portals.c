#include "game.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)
static void place(Actor *a,int x,int y){*a=(Actor){.x=(float)x,.y=(float)y,.cx=x,.cy=y,.tx=x,.ty=y,.dir=DIR_LEFT};}
static Game arena(void){Game g;game_init(&g,123,true);for(int i=0;i<MAX_MONSTERS;i++)g.monsters[i].alive=false;place(&g.player,5,3);return g;}
static bool position(const Actor *a,int x,int y){return a->x==x&&a->y==y&&a->cx==x&&a->cy==y&&a->tx==x&&a->ty==y&&!a->moving&&a->dir==DIR_LEFT;}

int main(void){
    int checks=0,failures=0;
    Game core;game_init(&core,123,false);
    CHECK(!core.portals_enabled&&!core.portal_latched);
    CHECK(core.map[3][5]==TILE_FLOOR&&core.map[9][9]==TILE_FLOOR);
    Game g=arena();CHECK(g.map[3][5]==TILE_PORTAL_A&&g.map[9][9]==TILE_PORTAL_B);
    game_try_portal(&g);CHECK(position(&g.player,9,9));
    CHECK(g.portal_latched&&g.portal_cooldown==PORTAL_COOLDOWN);
    g.portal_cooldown=0;game_try_portal(&g);CHECK(position(&g.player,9,9));
    for(int i=0;i<240;i++)game_tick(&g,(Input){DIR_NONE,false});
    CHECK(position(&g.player,9,9)&&g.portal_latched&&g.portal_cooldown==0);
    place(&g.player,8,9);game_try_portal(&g);CHECK(!g.portal_latched);
    place(&g.player,9,9);game_try_portal(&g);CHECK(position(&g.player,5,3)&&g.portal_latched);
    for(int reason=0;reason<10;reason++){
        g=arena();
        switch(reason){
            case 0:g.portals_enabled=false;break;
            case 1:g.phase=LEVEL_FLASH;break;
            case 2:g.phase=GAME_OVER;break;
            case 3:g.phase=ALL_CLEAR;break;
            case 4:g.stun=1;break;
            case 5:g.player.moving=true;break;
            case 6:g.portal_cooldown=0.1f;break;
            case 7:g.stage_invalid=true;break;
            case 8:g.player.x+=0.1f;break;
            case 9:g.map[9][9]=TILE_FLOOR;break;
        }
        game_try_portal(&g);CHECK(g.player.cx==5&&g.player.cy==3&&!g.portal_latched);
    }
    for(int unsafe=0;unsafe<3;unsafe++){
        g=arena();
        if(unsafe==0)g.bombs[0]=(Bomb){9,9,3,true};
        if(unsafe==1)g.flame[9][9]=FLAME_TIME;
        if(unsafe==2){g.monsters[0].alive=true;place(&g.monsters[0].body,9,9);g.monsters[0].body.x+=0.4f;}
        game_try_portal(&g);CHECK(position(&g.player,5,3)&&!g.portal_latched);
        g.bombs[0].active=false;g.flame[9][9]=0;g.monsters[0].alive=false;
        game_try_portal(&g);CHECK(position(&g.player,9,9)&&g.portal_latched);
    }
    g=arena();g.monsters[0].alive=true;place(&g.monsters[0].body,9,9);g.monsters[0].body.x+=0.6f;
    game_try_portal(&g);CHECK(position(&g.player,9,9));
    const int bad[][2]={{-1,3},{MAP_W,3},{5,-1},{5,MAP_H},{INT_MIN,INT_MAX}};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];i++){
        g=arena();g.player.cx=bad[i][0];g.player.cy=bad[i][1];game_try_portal(&g);
        CHECK(g.player.cx==bad[i][0]&&g.player.cy==bad[i][1]);
    }
    g=arena();g.player.x=NAN;game_try_portal(&g);CHECK(isnan(g.player.x)&&!g.portal_latched);
    g=arena();g.player.y=INFINITY;game_try_portal(&g);CHECK(isinf(g.player.y)&&!g.portal_latched);
    g=arena();game_try_portal(&g);g.phase=PLAYING;g.level=1;place(&g.player,13,11);game_check_exit(&g);
    CHECK(g.level==2&&g.portals_enabled&&!g.portal_latched&&g.portal_cooldown==0);
    CHECK(g.map[3][5]==TILE_PORTAL_A&&g.map[9][9]==TILE_PORTAL_B);
    game_init(&g,123,false);CHECK(!g.portals_enabled&&!g.portal_latched&&g.map[3][5]==TILE_FLOOR);
    printf("Portals: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
