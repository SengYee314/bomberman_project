#include "rules.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool matches(BmBlastResult got,bool valid,const bool flame[MAP_H][MAP_W],const bool brick[MAP_H][MAP_W]){
    int flames=0,bricks=0;
    if(got.valid!=valid)return false;
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
        if(got.flame[y][x]!=flame[y][x]||got.destroy_brick[y][x]!=brick[y][x])return false;
        flames+=flame[y][x];bricks+=brick[y][x];
    }
    return got.flame_count==flames&&got.brick_count==bricks;
}

int main(void){
    int checks=0,failures=0;
    Tile map[MAP_H][MAP_W]={{TILE_FLOOR}},before[MAP_H][MAP_W];
    bool flame[MAP_H][MAP_W],brick[MAP_H][MAP_W]={{false}},empty[MAP_H][MAP_W]={{false}};
    /* Independent geometric cross on an unobstructed map, clipped at every edge. */
    for(int cy=0;cy<MAP_H;cy++)for(int cx=0;cx<MAP_W;cx++)for(int range=1;range<=4;range++){
        for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
            flame[y][x]=(x==cx&&abs(y-cy)<=range)||(y==cy&&abs(x-cx)<=range);
        }
        memcpy(before,map,sizeof map);
        CHECK(matches(bm_trace_blast(BM_MAP_VIEW(map),cx,cy,range),true,BM_FLAME_VIEW(flame),BM_FLAME_VIEW(brick)));
        CHECK(memcmp(before,map,sizeof map)==0);
    }
    const int dx[4]={0,1,0,-1},dy[4]={1,0,-1,0};
    const Tile blockers[]={TILE_WALL,TILE_BRICK,(Tile)-1,(Tile)INT_MAX};
    for(int d=0;d<4;d++)for(int distance=1;distance<=4;distance++){
        for(size_t b=0;b<sizeof blockers/sizeof blockers[0];b++){
            memset(map,0,sizeof map);memset(brick,0,sizeof brick);
            for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
                flame[y][x]=(x==7&&abs(y-6)<=4)||(y==6&&abs(x-7)<=4);
            }
            map[6+dy[d]*distance][7+dx[d]*distance]=blockers[b];
            bool is_brick=blockers[b]==TILE_BRICK;
            if(is_brick)brick[6+dy[d]*distance][7+dx[d]*distance]=true;
            for(int n=distance+(is_brick?1:0);n<=4;n++)flame[6+dy[d]*n][7+dx[d]*n]=false;
            memcpy(before,map,sizeof map);
            CHECK(matches(bm_trace_blast(BM_MAP_VIEW(map),7,6,4),true,BM_FLAME_VIEW(flame),BM_FLAME_VIEW(brick)));
            CHECK(memcmp(before,map,sizeof map)==0);
        }
    }
    memset(map,0,sizeof map);
    const Tile centers[]={TILE_FLOOR,TILE_EXIT,TILE_PORTAL_A,TILE_PORTAL_B,TILE_WALL,TILE_BRICK,(Tile)INT_MAX};
    for(size_t i=0;i<sizeof centers/sizeof centers[0];i++){
        map[6][7]=centers[i];
        for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)flame[y][x]=(x==7&&abs(y-6)<=1)||(y==6&&abs(x-7)<=1);
        bool valid=i<4;
        CHECK(matches(bm_trace_blast(BM_MAP_VIEW(map),7,6,1),valid,
                      valid?BM_FLAME_VIEW(flame):BM_FLAME_VIEW(empty),BM_FLAME_VIEW(empty)));
    }
    memset(map,0,sizeof map);map[6][8]=TILE_BRICK;map[6][9]=TILE_BRICK;
    BmBlastResult two=bm_trace_blast(BM_MAP_VIEW(map),7,6,4);
    CHECK(two.brick_count==1&&two.destroy_brick[6][8]&&!two.flame[6][9]);
    const int ranges[]={0,5,-1,INT_MIN,INT_MAX};
    for(size_t i=0;i<sizeof ranges/sizeof ranges[0];i++){
        CHECK(matches(bm_trace_blast(BM_MAP_VIEW(map),7,6,ranges[i]),false,BM_FLAME_VIEW(empty),BM_FLAME_VIEW(empty)));
    }
    const int outside[][2]={{-1,0},{0,-1},{MAP_W,0},{0,MAP_H},{INT_MIN,1},{INT_MAX,1},{1,INT_MIN},{1,INT_MAX}};
    for(size_t i=0;i<sizeof outside/sizeof outside[0];i++){
        CHECK(matches(bm_trace_blast(BM_MAP_VIEW(map),outside[i][0],outside[i][1],4),false,BM_FLAME_VIEW(empty),BM_FLAME_VIEW(empty)));
    }
    printf("Trace blast: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
