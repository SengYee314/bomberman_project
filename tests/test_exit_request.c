#include "rules.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool same_exit(BmExitResult a,BmExitResult b){
    return a.status==b.status&&a.next_level==b.next_level&&a.phase==b.phase&&a.flash==b.flash&&a.reset_map==b.reset_map;
}

int main(void){
    int checks=0,failures=0;
    const BmExitResult success[]={
        {BM_EXIT_ADVANCE,2,LEVEL_FLASH,LEVEL_FLASH_TIME,true},
        {BM_EXIT_ADVANCE,3,LEVEL_FLASH,LEVEL_FLASH_TIME,true},
        {BM_EXIT_COMPLETE,3,ALL_CLEAR,0,false}
    };
    const float clear_stun[]={0,-0.0f,BM_EPSILON/2,BM_EPSILON};
    const Tile blocked_tiles[]={TILE_FLOOR,TILE_WALL,TILE_BRICK,TILE_PORTAL_A,TILE_PORTAL_B,(Tile)-1,(Tile)INT_MAX};
    const float blocked_stun[]={nextafterf(BM_EPSILON,INFINITY),1,FLT_MAX};
    const Phase blocked_phases[]={LEVEL_FLASH,GAME_OVER,ALL_CLEAR};
    for(int level=1;level<=3;level++){
        BmExitResult blocked={BM_EXIT_BLOCKED,level,PLAYING,0,false};
        for(size_t i=0;i<sizeof clear_stun/sizeof clear_stun[0];i++){
            CHECK(same_exit(bm_exit_request(level,TILE_EXIT,false,clear_stun[i],PLAYING),success[level-1]));
        }
        for(size_t i=0;i<sizeof blocked_tiles/sizeof blocked_tiles[0];i++){
            CHECK(same_exit(bm_exit_request(level,blocked_tiles[i],false,0,PLAYING),blocked));
        }
        CHECK(same_exit(bm_exit_request(level,TILE_EXIT,true,0,PLAYING),blocked));
        for(size_t i=0;i<sizeof blocked_stun/sizeof blocked_stun[0];i++){
            CHECK(same_exit(bm_exit_request(level,TILE_EXIT,false,blocked_stun[i],PLAYING),blocked));
        }
        for(size_t i=0;i<sizeof blocked_phases/sizeof blocked_phases[0];i++){
            BmExitResult expected={BM_EXIT_BLOCKED,level,blocked_phases[i],0,false};
            CHECK(same_exit(bm_exit_request(level,TILE_EXIT,false,0,blocked_phases[i]),expected));
            CHECK(same_exit(bm_exit_request(level,TILE_FLOOR,true,5,blocked_phases[i]),expected));
        }
    }
    const int bad_levels[]={-1,0,4,INT_MIN,INT_MAX};
    for(size_t i=0;i<sizeof bad_levels/sizeof bad_levels[0];i++){
        CHECK(same_exit(bm_exit_request(bad_levels[i],TILE_EXIT,false,0,PLAYING),
                        (BmExitResult){BM_EXIT_INVALID,bad_levels[i],PLAYING,0,false}));
        CHECK(same_exit(bm_exit_request(bad_levels[i],TILE_FLOOR,true,5,ALL_CLEAR),
                        (BmExitResult){BM_EXIT_INVALID,bad_levels[i],ALL_CLEAR,0,false}));
    }
    const float bad_stun[]={-1,-BM_EPSILON,NAN,INFINITY,-INFINITY};
    for(size_t i=0;i<sizeof bad_stun/sizeof bad_stun[0];i++){
        CHECK(same_exit(bm_exit_request(2,TILE_EXIT,false,bad_stun[i],PLAYING),
                        (BmExitResult){BM_EXIT_INVALID,2,PLAYING,0,false}));
    }
    const Phase bad_phases[]={(Phase)-1,(Phase)4,(Phase)INT_MIN,(Phase)INT_MAX};
    for(size_t i=0;i<sizeof bad_phases/sizeof bad_phases[0];i++){
        CHECK(same_exit(bm_exit_request(2,TILE_EXIT,false,0,bad_phases[i]),
                        (BmExitResult){BM_EXIT_INVALID,2,bad_phases[i],0,false}));
    }
    CHECK(same_exit(bm_exit_request(INT_MAX,(Tile)INT_MIN,true,NAN,(Phase)INT_MIN),
                    (BmExitResult){BM_EXIT_INVALID,INT_MAX,(Phase)INT_MIN,0,false}));
    printf("Exit request: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
