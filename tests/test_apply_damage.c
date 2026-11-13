#include "rules.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool same_damage(BmDamageResult a,BmDamageResult b){
    return a.valid==b.valid&&a.applied==b.applied&&a.lives==b.lives&&a.score==b.score&&
           (a.stun==b.stun||(isnan(a.stun)&&isnan(b.stun)))&&a.phase==b.phase;
}
static BmDamageResult preserved(BmDamageInput in,bool valid){
    return (BmDamageResult){valid,false,in.lives,in.score,in.stun,in.phase};
}

int main(void){
    int checks=0,failures=0;
    const BmScoreEvent causes[]={BM_SCORE_KILL,BM_SCORE_CONTACT,BM_SCORE_BOMB};
    const int deltas[]={10,-10,-20},scores[]={-1000000,0,1000000};
    /* Every allowed positive life count and all score endpoints/declared causes. */
    for(int lives=1;lives<=99;lives++)for(int c=0;c<3;c++)for(int s=0;s<3;s++){
        BmDamageInput in={lives,scores[s],BM_EPSILON,BM_EPSILON,PLAYING,causes[c]};
        BmDamageResult expected={true,true,lives-1,scores[s]+deltas[c],5,lives==1?GAME_OVER:PLAYING};
        CHECK(same_damage(bm_apply_damage(in),expected));
    }
    BmDamageInput base={3,25,0,0,PLAYING,BM_SCORE_BOMB};
    CHECK(same_damage(bm_apply_damage(base),(BmDamageResult){true,true,2,5,5,PLAYING}));
    base.stun=-0.0f;base.grace=-0.0f;
    CHECK(same_damage(bm_apply_damage(base),(BmDamageResult){true,true,2,5,5,PLAYING}));
    base.stun=0;base.grace=0;
    const Phase inactive[]={LEVEL_FLASH,GAME_OVER,ALL_CLEAR};
    for(size_t i=0;i<sizeof inactive/sizeof inactive[0];i++){
        BmDamageInput in=base;in.phase=inactive[i];
        CHECK(same_damage(bm_apply_damage(in),preserved(in,true)));
    }
    BmDamageInput zero=base;zero.lives=0;
    CHECK(same_damage(bm_apply_damage(zero),preserved(zero,true)));
    const float blocked[]={nextafterf(BM_EPSILON,INFINITY),1,FLT_MAX};
    for(size_t i=0;i<sizeof blocked/sizeof blocked[0];i++){
        BmDamageInput in=base;in.stun=blocked[i];
        CHECK(same_damage(bm_apply_damage(in),preserved(in,true)));
        in=base;in.grace=blocked[i];
        CHECK(same_damage(bm_apply_damage(in),preserved(in,true)));
    }
    const int bad_lives[]={-1,100,INT_MIN,INT_MAX};
    const int bad_scores[]={-1000001,1000001,INT_MIN,INT_MAX};
    const float bad_timers[]={-1,-BM_EPSILON,NAN,INFINITY,-INFINITY};
    const int bad_enums[]={-1,4,INT_MIN,INT_MAX};
    for(int pass=0;pass<2;pass++){
        /* Validation must happen even when the phase would ignore a valid hit. */
        base.phase=pass==0?PLAYING:GAME_OVER;
        for(size_t i=0;i<sizeof bad_lives/sizeof bad_lives[0];i++){
            BmDamageInput in=base;in.lives=bad_lives[i];
            CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
        }
        for(size_t i=0;i<sizeof bad_scores/sizeof bad_scores[0];i++){
            BmDamageInput in=base;in.score=bad_scores[i];
            CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
        }
        for(size_t i=0;i<sizeof bad_timers/sizeof bad_timers[0];i++){
            BmDamageInput in=base;in.stun=bad_timers[i];
            CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
            in=base;in.grace=bad_timers[i];
            CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
        }
        for(size_t i=0;i<sizeof bad_enums/sizeof bad_enums[0];i++){
            BmDamageInput in=base;in.phase=(Phase)bad_enums[i];
            CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
            in=base;in.cause=(BmScoreEvent)bad_enums[i];
            CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
        }
        BmDamageInput in=base;in.cause=(BmScoreEvent)3;
        CHECK(same_damage(bm_apply_damage(in),preserved(in,false)));
    }
    BmDamageInput all_bad={INT_MAX,INT_MIN,NAN,NAN,(Phase)INT_MAX,(BmScoreEvent)INT_MIN};
    CHECK(same_damage(bm_apply_damage(all_bad),preserved(all_bad,false)));
    BmDamageResult hit=bm_apply_damage((BmDamageInput){3,0,0,0,PLAYING,BM_SCORE_CONTACT});
    BmDamageInput repeated={hit.lives,hit.score,hit.stun,0,hit.phase,BM_SCORE_CONTACT};
    CHECK(same_damage(bm_apply_damage(repeated),preserved(repeated,true)));
    printf("Apply damage: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
