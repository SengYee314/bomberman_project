#include "rules.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool same_move(BmMoveDecision a,BmMoveDecision b){
    return a.start==b.start&&a.direction==b.direction&&a.target_x==b.target_x&&a.target_y==b.target_y;
}

int main(void){
    int checks=0,failures=0;
    Tile map[MAP_H][MAP_W],before_map[MAP_H][MAP_W];
    bool bombs[MAP_H][MAP_W]={{false}},before_bombs[MAP_H][MAP_W];
    const struct {
        const char *name;
        Direction facing,requested;
        bool moving,stunned;
        unsigned open_mask;
        BmMoveDecision expected;
    } cases[]={
        {"down",DIR_UP,DIR_DOWN,false,false,15,{true,DIR_DOWN,7,7}},
        {"right",DIR_DOWN,DIR_RIGHT,false,false,15,{true,DIR_RIGHT,8,6}},
        {"up",DIR_LEFT,DIR_UP,false,false,15,{true,DIR_UP,7,5}},
        {"left",DIR_RIGHT,DIR_LEFT,false,false,15,{true,DIR_LEFT,6,6}},
        {"fallback down",DIR_DOWN,DIR_RIGHT,false,false,1,{true,DIR_DOWN,7,7}},
        {"fallback right",DIR_RIGHT,DIR_UP,false,false,2,{true,DIR_RIGHT,8,6}},
        {"fallback up",DIR_UP,DIR_LEFT,false,false,4,{true,DIR_UP,7,5}},
        {"fallback left",DIR_LEFT,DIR_DOWN,false,false,8,{true,DIR_LEFT,6,6}},
        {"no sideways search",DIR_DOWN,DIR_RIGHT,false,false,4,{false,DIR_DOWN,7,6}},
        {"all blocked",DIR_LEFT,DIR_UP,false,false,0,{false,DIR_LEFT,7,6}},
        {"already moving",DIR_DOWN,DIR_RIGHT,true,false,15,{false,DIR_DOWN,7,6}},
        {"stunned",DIR_UP,DIR_LEFT,false,true,15,{false,DIR_UP,7,6}},
        {"moving and stunned",DIR_LEFT,DIR_DOWN,true,true,15,{false,DIR_LEFT,7,6}},
        {"no request",DIR_RIGHT,DIR_NONE,false,false,15,{false,DIR_RIGHT,7,6}},
        {"negative request",DIR_LEFT,(Direction)-1,false,false,15,{false,DIR_LEFT,7,6}},
        {"minimum request",DIR_LEFT,(Direction)INT_MIN,false,false,15,{false,DIR_LEFT,7,6}},
        {"maximum request",DIR_LEFT,(Direction)INT_MAX,false,false,15,{false,DIR_LEFT,7,6}},
        {"request works with no facing",DIR_NONE,DIR_RIGHT,false,false,2,{true,DIR_RIGHT,8,6}},
        {"request works with invalid facing",(Direction)INT_MAX,DIR_UP,false,false,4,{true,DIR_UP,7,5}},
        {"invalid fallback stays unchanged",(Direction)-1,DIR_RIGHT,false,false,1,{false,(Direction)-1,7,6}},
        {"minimum fallback stays unchanged",(Direction)INT_MIN,DIR_RIGHT,false,false,1,{false,(Direction)INT_MIN,7,6}},
        {"none fallback stays unchanged",DIR_NONE,DIR_RIGHT,false,false,1,{false,DIR_NONE,7,6}}
    };
    const int dx[4]={0,1,0,-1},dy[4]={1,0,-1,0};
    for(size_t i=0;i<sizeof cases/sizeof cases[0];i++){
        for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)map[y][x]=TILE_WALL;
        map[6][7]=TILE_FLOOR;
        for(int d=0;d<4;d++)if(cases[i].open_mask&(1u<<d))map[6+dy[d]][7+dx[d]]=TILE_FLOOR;
        memcpy(before_map,map,sizeof map);memcpy(before_bombs,bombs,sizeof bombs);
        BmMoveDecision got=bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),7,6,
                cases[i].facing,cases[i].requested,cases[i].moving,cases[i].stunned);
        checks++;
        if(!same_move(got,cases[i].expected)){printf("FAIL %s\n",cases[i].name);failures++;}
        CHECK(memcmp(before_map,map,sizeof map)==0);
        CHECK(memcmp(before_bombs,bombs,sizeof bombs)==0);
    }

    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)map[y][x]=TILE_FLOOR;
    const int outside[][2]={{-1,1},{1,-1},{MAP_W,1},{1,MAP_H},
                            {INT_MIN,1},{INT_MAX,1},{1,INT_MIN},{1,INT_MAX}};
    for(size_t i=0;i<sizeof outside/sizeof outside[0];i++){
        BmMoveDecision expected={false,DIR_DOWN,outside[i][0],outside[i][1]};
        CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),outside[i][0],outside[i][1],
                                      DIR_DOWN,DIR_RIGHT,false,false),expected));
    }
    CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),0,0,DIR_LEFT,DIR_UP,false,false),
                    (BmMoveDecision){false,DIR_LEFT,0,0}));
    CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),MAP_W-1,MAP_H-1,DIR_DOWN,DIR_RIGHT,false,false),
                    (BmMoveDecision){false,DIR_DOWN,MAP_W-1,MAP_H-1}));

    const Tile blockers[]={TILE_WALL,TILE_BRICK,(Tile)INT_MAX};
    for(size_t i=0;i<sizeof blockers/sizeof blockers[0];i++){
        map[6][8]=blockers[i];
        CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),7,6,DIR_DOWN,DIR_RIGHT,false,false),
                        (BmMoveDecision){true,DIR_DOWN,7,7}));
    }
    map[6][8]=TILE_FLOOR;bombs[6][8]=true;
    CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),7,6,DIR_DOWN,DIR_RIGHT,false,false),
                    (BmMoveDecision){true,DIR_DOWN,7,7}));
    bombs[6][8]=false;bombs[6][7]=true;
    CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),7,6,DIR_DOWN,DIR_RIGHT,false,false),
                    (BmMoveDecision){true,DIR_RIGHT,8,6}));
    const Tile destinations[]={TILE_EXIT,TILE_PORTAL_A,TILE_PORTAL_B};
    for(size_t i=0;i<sizeof destinations/sizeof destinations[0];i++){
        map[6][8]=destinations[i];
        CHECK(same_move(bm_player_step(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),7,6,DIR_DOWN,DIR_RIGHT,false,false),
                        (BmMoveDecision){true,DIR_RIGHT,8,6}));
    }
    printf("Player step: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
