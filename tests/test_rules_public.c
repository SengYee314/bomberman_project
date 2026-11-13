#include "rules.h"
#include <math.h>
#include <stdio.h>
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);failures++;}}while(0)
int main(void){
 int failures=0;
 Tile map[MAP_H][MAP_W]={{TILE_FLOOR}};bool bombs[MAP_H][MAP_W]={{false}};
 CHECK(bm_can_enter(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),1,1));
 map[1][1]=TILE_WALL;CHECK(!bm_can_enter(BM_MAP_VIEW(map),BM_BOMB_VIEW(bombs),1,1));
 BmLevelRules level=bm_level_rules(3);CHECK(level.valid&&fabsf(level.player_speed-4.8f)<.0001f&&level.blast_range==4);
 BmBombDecision bomb=bm_bomb_request((BmBombRequest){true,false,false,1,0});CHECK(bomb.allowed&&bomb.status==BM_PLACE_OK&&bomb.fuse==3&&bomb.cooldown==5);
 BmDamageResult hit=bm_apply_damage((BmDamageInput){3,0,0,0,PLAYING,BM_SCORE_BOMB});CHECK(hit.valid&&hit.applied&&hit.lives==2&&hit.score==-20&&hit.stun==5&&hit.phase==PLAYING);
 BmTimerResult timer=bm_tick_timer(.5f,.5f);CHECK(timer.valid&&timer.just_expired&&timer.remaining==0);
 bool flame[MAP_H][MAP_W]={{false}};flame[5][6]=true;
 CHECK(bm_flame_hit(BM_FLAME_VIEW(flame),6,5,true)&&!bm_flame_hit(BM_FLAME_VIEW(flame),6,5,false)&&!bm_flame_hit(BM_FLAME_VIEW(flame),4,5,true));
 BmStageStart start=bm_stage_start(true,0,0,0);CHECK(start.valid&&start.level==1&&start.lives==3&&start.score==0&&start.time_left==START_TIME);
 BmFlashResult flash=bm_level_flash(LEVEL_FLASH_TIME,0);CHECK(flash.valid&&flash.white&&flash.phase==LEVEL_FLASH);
 printf("Public examples: %d failed\n",failures);
#ifdef SUBMISSION_CHECK
 /* Reaching here means rules.c built against the original headers and engine, as the grader does. */
 printf("Contract compile OK: src/rules.c builds with the original headers and engine.\n");
 printf("The private grader also checks pure-function policy (see rules.h).\n");
 printf("This check does not guarantee full marks in the private grader.\n");
 if(failures)printf("Submission check FAILED: %d public examples failed. Fix the failures above.\n",failures);
 else printf("Submission check OK: contract compilation and public examples passed.\n");
#endif
 return failures?1:0;
}
