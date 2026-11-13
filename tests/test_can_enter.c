#include "rules.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

int main(void){
    int checks=0,failures=0;
    Tile map[MAP_H][MAP_W],before_map[MAP_H][MAP_W];
    bool bombs[MAP_H][MAP_W]={{false}},before_bombs[MAP_H][MAP_W];
    const Tile tiles[]={TILE_FLOOR,TILE_EXIT,TILE_PORTAL_A,TILE_PORTAL_B,
                        TILE_WALL,TILE_BRICK,(Tile)-1,(Tile)INT_MAX};
    const bool enterable[]={true,true,true,true,false,false,false,false};
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)map[y][x]=TILE_WALL;

    /* Every cell, including the non-square map's last column and row. */
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
        for(size_t t=0;t<sizeof tiles/sizeof tiles[0];t++){
            for(int occupied=0;occupied<2;occupied++){
                map[y][x]=tiles[t];bombs[y][x]=occupied!=0;
                memcpy(before_map,map,sizeof map);
                memcpy(before_bombs,bombs,sizeof bombs);
                CHECK(bm_can_enter(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),x,y)==
                      (enterable[t]&&occupied==0));
                CHECK(memcmp(before_map,map,sizeof map)==0);
                CHECK(memcmp(before_bombs,bombs,sizeof bombs)==0);
            }
        }
        map[y][x]=TILE_WALL;bombs[y][x]=false;
    }

    const int outside[][2]={{-1,0},{0,-1},{MAP_W,0},{0,MAP_H},
                            {MAP_W,MAP_H},{INT_MIN,1},{INT_MAX,1},
                            {1,INT_MIN},{1,INT_MAX},{INT_MIN,INT_MAX}};
    for(size_t i=0;i<sizeof outside/sizeof outside[0];i++){
        CHECK(!bm_can_enter(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),outside[i][0],outside[i][1]));
    }
    printf("Can enter: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
