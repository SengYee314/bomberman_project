#include "rules.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool same_timer(BmTimerResult got,BmTimerResult expected){
    return got.valid==expected.valid&&got.just_expired==expected.just_expired&&got.remaining==expected.remaining;
}

int main(void){
    int checks=0,failures=0;
    const struct {float remaining,dt;BmTimerResult expected;} cases[]={
        {1,0,{true,false,1}},
        {1,0.25f,{true,false,0.75f}},
        {0.5f,0.5f,{true,true,0}},
        {0.5f,1,{true,true,0}},
        {0,0,{true,false,0}},
        {0,1,{true,false,0}},
        {-0.0f,0,{true,false,0}},
        {1,-0.0f,{true,false,1}},
        {BM_EPSILON,0,{true,true,0}},
        {BM_EPSILON/2,0,{true,true,0}},
        {2*BM_EPSILON,BM_EPSILON,{true,true,0}},
        {4*BM_EPSILON,2*BM_EPSILON,{true,false,2*BM_EPSILON}},
        {FLT_MAX,0,{true,false,FLT_MAX}},
        {FLT_MAX,FLT_MAX,{true,true,0}},
        {0,FLT_MAX,{true,false,0}},
        {1,FLT_MAX,{true,true,0}}
    };
    for(size_t i=0;i<sizeof cases/sizeof cases[0];i++){
        CHECK(same_timer(bm_tick_timer(cases[i].remaining,cases[i].dt),cases[i].expected));
    }
    float below=nextafterf(BM_EPSILON,0.0f),above=nextafterf(BM_EPSILON,INFINITY);
    CHECK(same_timer(bm_tick_timer(below,0),(BmTimerResult){true,true,0}));
    CHECK(same_timer(bm_tick_timer(above,0),(BmTimerResult){true,false,above}));
    CHECK(same_timer(bm_tick_timer(nextafterf(0.0f,1.0f),0),(BmTimerResult){true,true,0}));

    const float invalid[]={-1,-BM_EPSILON,-FLT_MAX,NAN,INFINITY,-INFINITY};
    const float valid[]={0,1,FLT_MAX};
    for(size_t i=0;i<sizeof invalid/sizeof invalid[0];i++){
        for(size_t j=0;j<sizeof valid/sizeof valid[0];j++){
            CHECK(same_timer(bm_tick_timer(invalid[i],valid[j]),(BmTimerResult){0}));
            CHECK(same_timer(bm_tick_timer(valid[j],invalid[i]),(BmTimerResult){0}));
        }
        for(size_t j=0;j<sizeof invalid/sizeof invalid[0];j++){
            CHECK(same_timer(bm_tick_timer(invalid[i],invalid[j]),(BmTimerResult){0}));
        }
    }

    /* Real engine cadence: fuse 3 seconds and cooldown/stun 5 seconds. */
    const struct {float duration;int ticks;} runs[]={{3,360},{5,600}};
    for(size_t run=0;run<sizeof runs/sizeof runs[0];run++){
        float remaining=runs[run].duration;
        int expirations=0;
        for(int tick=1;tick<=runs[run].ticks;tick++){
            BmTimerResult result=bm_tick_timer(remaining,FIXED_DT);
            CHECK(result.valid);
            CHECK(result.just_expired==(tick==runs[run].ticks));
            CHECK(tick==runs[run].ticks?result.remaining==0:result.remaining>0);
            remaining=result.remaining;expirations+=result.just_expired;
        }
        CHECK(expirations==1);
        CHECK(same_timer(bm_tick_timer(remaining,FIXED_DT),(BmTimerResult){true,false,0}));
    }
    BmTimerResult first=bm_tick_timer(10,0.25f);
    CHECK(same_timer(bm_tick_timer(first.remaining,0.5f),bm_tick_timer(10,0.75f)));
    printf("Tick timer: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
