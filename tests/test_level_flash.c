#include "rules.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool same_flash(BmFlashResult a,BmFlashResult b){
    return a.valid==b.valid&&a.white==b.white&&a.remaining==b.remaining&&a.phase==b.phase;
}

int main(void){
    int checks=0,failures=0;
    CHECK(same_flash(bm_level_flash(LEVEL_FLASH_TIME,0),(BmFlashResult){true,true,LEVEL_FLASH_TIME,LEVEL_FLASH}));
    CHECK(same_flash(bm_level_flash(0,0),(BmFlashResult){true,false,0,PLAYING}));
    CHECK(same_flash(bm_level_flash(-0.0f,-0.0f),(BmFlashResult){true,false,0,PLAYING}));
    CHECK(same_flash(bm_level_flash(BM_EPSILON,0),(BmFlashResult){true,false,0,PLAYING}));
    CHECK(same_flash(bm_level_flash(BM_EPSILON/2,0),(BmFlashResult){true,false,0,PLAYING}));
    float above=nextafterf(BM_EPSILON,INFINITY);
    CHECK(same_flash(bm_level_flash(above,0),(BmFlashResult){true,false,above,LEVEL_FLASH}));
    const bool whites[]={true,false,true,false,true,false};
    for(int band=0;band<6;band++){
        float remaining=LEVEL_FLASH_TIME-((float)band+0.5f)*FLASH_BLINK;
        CHECK(same_flash(bm_level_flash(remaining,0),(BmFlashResult){true,whites[band],remaining,LEVEL_FLASH}));
    }
    /* Stay well outside the contract's excluded +/-1e-4 blink boundaries. */
    for(int edge=1;edge<6;edge++){
        float before=LEVEL_FLASH_TIME-((float)edge*FLASH_BLINK-0.001f);
        float after=LEVEL_FLASH_TIME-((float)edge*FLASH_BLINK+0.001f);
        CHECK(same_flash(bm_level_flash(before,0),(BmFlashResult){true,whites[edge-1],before,LEVEL_FLASH}));
        CHECK(same_flash(bm_level_flash(after,0),(BmFlashResult){true,whites[edge],after,LEVEL_FLASH}));
    }
    BmFlashResult jump=bm_level_flash(LEVEL_FLASH_TIME,0.65f);
    CHECK(jump.valid&&jump.white&&jump.phase==LEVEL_FLASH&&jump.remaining==LEVEL_FLASH_TIME-0.65f);
    CHECK(same_flash(bm_level_flash(LEVEL_FLASH_TIME,LEVEL_FLASH_TIME),(BmFlashResult){true,false,0,PLAYING}));
    CHECK(same_flash(bm_level_flash(LEVEL_FLASH_TIME,FLT_MAX),(BmFlashResult){true,false,0,PLAYING}));
    const float bad_remaining[]={-1,-BM_EPSILON,nextafterf(LEVEL_FLASH_TIME,INFINITY),1,FLT_MAX,NAN,INFINITY,-INFINITY};
    for(size_t i=0;i<sizeof bad_remaining/sizeof bad_remaining[0];i++){
        CHECK(same_flash(bm_level_flash(bad_remaining[i],0),(BmFlashResult){false,false,0,PLAYING}));
    }
    const float bad_dt[]={-1,-BM_EPSILON,NAN,INFINITY,-INFINITY};
    for(size_t i=0;i<sizeof bad_dt/sizeof bad_dt[0];i++){
        CHECK(same_flash(bm_level_flash(LEVEL_FLASH_TIME,bad_dt[i]),(BmFlashResult){false,false,0,PLAYING}));
    }
    float remaining=LEVEL_FLASH_TIME;
    int white_runs=0;bool was_white=false;
    for(int tick=0;tick<108;tick++){
        BmFlashResult query=bm_level_flash(remaining,0);
        CHECK(query.valid&&query.phase==LEVEL_FLASH&&query.remaining==remaining);
        white_runs+=query.white&&!was_white;was_white=query.white;
        BmFlashResult next=bm_level_flash(remaining,FIXED_DT);
        CHECK(next.valid);
        CHECK(tick==107?same_flash(next,(BmFlashResult){true,false,0,PLAYING}):next.phase==LEVEL_FLASH&&next.remaining>0);
        remaining=next.remaining;
    }
    CHECK(white_runs==3);
    CHECK(same_flash(bm_level_flash(remaining,FIXED_DT),(BmFlashResult){true,false,0,PLAYING}));
    printf("Level flash: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
