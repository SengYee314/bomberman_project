#include "rules.h"
#include <limits.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

int main(void){
    int checks=0,failures=0;
    const struct {BmScoreEvent event;int expected;} cases[]={
        {BM_SCORE_KILL,10},{BM_SCORE_CONTACT,-10},{BM_SCORE_BOMB,-20},
        {(BmScoreEvent)-1,0},{(BmScoreEvent)3,0},{(BmScoreEvent)99,0},
        {(BmScoreEvent)INT_MIN,0},{(BmScoreEvent)INT_MAX,0}
    };
    for(int pass=0;pass<2;pass++)for(size_t i=0;i<sizeof cases/sizeof cases[0];i++){
        size_t index=pass==0?i:sizeof cases/sizeof cases[0]-1-i;
        CHECK(bm_score_delta(cases[index].event)==cases[index].expected);
    }
    CHECK(bm_score_delta(BM_SCORE_KILL)+bm_score_delta(BM_SCORE_CONTACT)+bm_score_delta(BM_SCORE_BOMB)==-20);
    printf("Score delta: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
