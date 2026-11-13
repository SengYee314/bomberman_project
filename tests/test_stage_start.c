#include "rules.h"
#include <limits.h>
#include <stdio.h>

typedef struct {
    const char *name;
    bool new_game;
    int level, lives, score;
    BmStageStart expected;
} StageCase;

int main(void){
    const StageCase cases[]={
        /* New games ignore both valid and invalid continuation values. */
        {"new game with zeros",true,0,0,0,{true,1,START_LIVES,0,START_TIME}},
        {"new game resets state",true,3,99,1000000,{true,1,START_LIVES,0,START_TIME}},
        {"new game ignores minimum integers",true,INT_MIN,INT_MIN,INT_MIN,{true,1,START_LIVES,0,START_TIME}},
        {"new game ignores maximum integers",true,INT_MAX,INT_MAX,INT_MAX,{true,1,START_LIVES,0,START_TIME}},
        {"new game ignores mixed extremes",true,INT_MAX,INT_MIN,INT_MAX,{true,1,START_LIVES,0,START_TIME}},

        /* Continuations preserve state, including inclusive endpoints. */
        {"level one is accepted",false,1,7,25,{true,1,7,25,START_TIME}},
        {"level two keeps negative score",false,2,2,-30,{true,2,2,-30,START_TIME}},
        {"level three stays level three",false,3,3,40,{true,3,3,40,START_TIME}},
        {"minimum lives",false,2,1,0,{true,2,1,0,START_TIME}},
        {"maximum lives",false,2,99,0,{true,2,99,0,START_TIME}},
        {"minimum score",false,2,3,-1000000,{true,2,3,-1000000,START_TIME}},
        {"maximum score",false,2,3,1000000,{true,2,3,1000000,START_TIME}},
        {"all lower endpoints",false,1,1,-1000000,{true,1,1,-1000000,START_TIME}},
        {"all upper endpoints",false,3,99,1000000,{true,3,99,1000000,START_TIME}},

        /* A single invalid field must zero the entire result. */
        {"level zero",false,0,3,50,{0}},
        {"level four",false,4,3,50,{0}},
        {"negative level",false,-1,3,50,{0}},
        {"minimum integer level",false,INT_MIN,3,50,{0}},
        {"maximum integer level",false,INT_MAX,3,50,{0}},
        {"zero lives",false,2,0,50,{0}},
        {"one hundred lives",false,2,100,50,{0}},
        {"negative lives",false,2,-1,50,{0}},
        {"minimum integer lives",false,2,INT_MIN,50,{0}},
        {"maximum integer lives",false,2,INT_MAX,50,{0}},
        {"score below minimum",false,2,3,-1000001,{0}},
        {"score above maximum",false,2,3,1000001,{0}},
        {"minimum integer score",false,2,3,INT_MIN,{0}},
        {"maximum integer score",false,2,3,INT_MAX,{0}},
        {"multiple invalid fields",false,0,0,1000001,{0}},
        {"all minimum integers",false,INT_MIN,INT_MIN,INT_MIN,{0}},
        {"all maximum integers",false,INT_MAX,INT_MAX,INT_MAX,{0}}
    };
    const size_t count=sizeof cases/sizeof cases[0];
    int passed=0,failed=0;

    /* Repeat in reverse order to check that previous calls leave no state. */
    for(int pass=0;pass<2;pass++){
        for(size_t i=0;i<count;i++){
            const StageCase *test=&cases[pass==0?i:count-1-i];
            BmStageStart got=bm_stage_start(test->new_game,test->level,test->lives,test->score);
            BmStageStart expected=test->expected;
            if(got.valid!=expected.valid||got.level!=expected.level||got.lives!=expected.lives||
               got.score!=expected.score||got.time_left!=expected.time_left){
                printf("FAIL %s (pass %d): got {%d,%d,%d,%d,%.9g}, expected {%d,%d,%d,%d,%.9g}\n",
                       test->name,pass+1,got.valid,got.level,got.lives,got.score,got.time_left,
                       expected.valid,expected.level,expected.lives,expected.score,expected.time_left);
                failed++;
            }else{
                passed++;
            }
        }
    }
    printf("Stage start: %d passed, %d failed\n",passed,failed);
    return failed?1:0;
}
