#include "rules.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>

typedef struct {
    int level;
    BmLevelRules expected;
} LevelCase;

static bool speed_matches(float actual,float expected){
    if(!isfinite(actual))return false;
    if(expected==0.0f)return actual==0.0f;
    return fabsf(actual-expected)<0.0001f;
}

int main(void){
    const LevelCase cases[]={
        {1,{true,3.0f,1.6f,2}},
        {2,{true,3.9f,1.92f,3}},
        {3,{true,4.8f,2.4f,4}},
        {0,{0}},
        {4,{0}},
        {-1,{0}},
        {5,{0}},
        {-100,{0}},
        {100,{0}},
        {INT_MIN,{0}},
        {INT_MAX,{0}}
    };
    const size_t count=sizeof cases/sizeof cases[0];
    int passed=0,failed=0;

    /* Repeat in reverse order to check that previous calls leave no state. */
    for(int pass=0;pass<2;pass++){
        for(size_t i=0;i<count;i++){
            const LevelCase *test=&cases[pass==0?i:count-1-i];
            BmLevelRules got=bm_level_rules(test->level);
            BmLevelRules expected=test->expected;
            if(got.valid!=expected.valid||got.blast_range!=expected.blast_range||
               !speed_matches(got.player_speed,expected.player_speed)||
               !speed_matches(got.monster_speed,expected.monster_speed)){
                printf("FAIL direct level %d (pass %d): got {%d,%.9g,%.9g,%d}, expected {%d,%.9g,%.9g,%d}\n",
                       test->level,pass+1,got.valid,got.player_speed,got.monster_speed,got.blast_range,
                       expected.valid,expected.player_speed,expected.monster_speed,expected.blast_range);
                failed++;
            }else{
                passed++;
            }

            /* Test the engine accessors directly, without needing working exits. */
            Game game={0};
            game.level=test->level;
            float player_speed=game_player_speed(&game);
            float monster_speed=game_monster_speed(&game);
            int blast_range=game_blast_range(&game);
            if(!speed_matches(player_speed,expected.player_speed)||
               !speed_matches(monster_speed,expected.monster_speed)||
               blast_range!=expected.blast_range){
                printf("FAIL engine level %d (pass %d): got {%.9g,%.9g,%d}, expected {%.9g,%.9g,%d}\n",
                       test->level,pass+1,player_speed,monster_speed,blast_range,
                       expected.player_speed,expected.monster_speed,expected.blast_range);
                failed++;
            }else{
                passed++;
            }
        }
    }
    printf("Level rules: %d passed, %d failed\n",passed,failed);
    return failed?1:0;
}
