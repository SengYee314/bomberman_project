#include "rules.h"
#include <math.h>
static const int RX[4]={0,1,0,-1},RY[4]={1,0,-1,0};
static bool pos_ok(int x,int y){return x>=0&&x<MAP_W&&y>=0&&y<MAP_H;}
static bool dir_ok(Direction d){return d>=DIR_DOWN&&d<=DIR_LEFT;}
static bool walkable(Tile t){return t==TILE_FLOOR||t==TILE_EXIT||t==TILE_PORTAL_A||t==TILE_PORTAL_B;}

bool bm_can_enter(const Tile map[MAP_H][MAP_W],const bool bombs[MAP_H][MAP_W],int x,int y){
    if(!pos_ok(x,y))return false;
    return walkable(map[y][x])&&!bombs[y][x];
}
BmMoveDecision bm_player_step(const Tile map[MAP_H][MAP_W],const bool bombs[MAP_H][MAP_W],int x,int y,Direction facing,Direction requested,bool moving,bool stunned){
    BmMoveDecision stopped={.start=false,.direction=facing,.target_x=x,.target_y=y};
    if(moving||stunned||!pos_ok(x,y)||!dir_ok(requested))return stopped;

    int tx=x+RX[requested],ty=y+RY[requested];
    if(bm_can_enter(map,bombs,tx,ty)){
        return (BmMoveDecision){.start=true,.direction=requested,.target_x=tx,.target_y=ty};
    }
    if(dir_ok(facing)){
        tx=x+RX[facing];ty=y+RY[facing];
        if(bm_can_enter(map,bombs,tx,ty)){
            return (BmMoveDecision){.start=true,.direction=facing,.target_x=tx,.target_y=ty};
        }
    }
    return stopped;
}
Direction bm_monster_direction(MonsterKind kind,Direction current,const bool open[4],uint32_t sample){
    switch(kind){
        case MONSTER_WHITE:
            if(current!=DIR_DOWN&&current!=DIR_UP)current=DIR_DOWN;
            break;
        case MONSTER_YELLOW:
            if(current!=DIR_LEFT&&current!=DIR_RIGHT)current=DIR_LEFT;
            break;
        case MONSTER_GREEN:
            if(!dir_ok(current))current=DIR_DOWN;
            break;
        default:
            return DIR_NONE;
    }
    Direction reverse=(Direction)((current+2)%4);
    if(kind!=MONSTER_GREEN){
        if(open[current])return current;
        return open[reverse]?reverse:DIR_NONE;
    }

    int open_count=0;
    for(int d=0;d<4;d++)if(open[d])open_count++;
    if(open_count<3){
        if(open[current])return current;
        if(open[reverse])return reverse;
    }
    Direction choices[4];
    int count=0;
    for(int d=0;d<4;d++)if(open[d]&&(Direction)d!=reverse)choices[count++]=(Direction)d;
    if(count==0)return DIR_NONE;
    return choices[sample%(uint32_t)count];
}
BmLevelRules bm_level_rules(int level){
    switch(level){
        case 1:
            return (BmLevelRules){.valid=true,.player_speed=3.0f,
                                  .monster_speed=1.6f,.blast_range=2};
        case 2:
            return (BmLevelRules){.valid=true,.player_speed=3.9f,
                                  .monster_speed=1.92f,.blast_range=3};
        case 3:
            return (BmLevelRules){.valid=true,.player_speed=4.8f,
                                  .monster_speed=2.4f,.blast_range=4};
        default:
            return (BmLevelRules){0};
    }
}
BmBombDecision bm_bomb_request(BmBombRequest r){
    bool cooldown_valid=isfinite(r.cooldown)&&r.cooldown>=0;
    BmBombDecision result={.allowed=false,.status=BM_PLACE_INVALID,.fuse=0,
                           .cooldown=cooldown_valid?r.cooldown:0};
    if(r.free_slots<0||r.free_slots>MAX_BOMBS||!cooldown_valid)return result;

    if(!r.game_active)result.status=BM_PLACE_INACTIVE;
    else if(r.stunned)result.status=BM_PLACE_STUNNED;
    else if(r.cooldown>0)result.status=BM_PLACE_COOLDOWN;
    else if(r.tile_has_bomb)result.status=BM_PLACE_DUPLICATE;
    else if(r.free_slots==0)result.status=BM_PLACE_FULL;
    else{
        result.allowed=true;result.status=BM_PLACE_OK;
        result.fuse=BOMB_FUSE;result.cooldown=BOMB_COOLDOWN;
    }
    return result;
}
BmTimerResult bm_tick_timer(float remaining,float dt){
    if(!isfinite(remaining)||!isfinite(dt)||remaining<0||dt<0)return (BmTimerResult){0};
    float next=remaining-dt;
    if(next<=BM_EPSILON)next=0;
    return (BmTimerResult){.valid=true,.just_expired=remaining>0&&next==0,.remaining=next};
}
BmBlastResult bm_trace_blast(const Tile map[MAP_H][MAP_W],int x,int y,int range){
    BmBlastResult result={0};
    if(range<1||range>4||!pos_ok(x,y)||!walkable(map[y][x]))return result;
    result.valid=true;result.flame[y][x]=true;result.flame_count=1;
    for(int d=0;d<4;d++){
        for(int step=1;step<=range;step++){
            int tx=x+RX[d]*step,ty=y+RY[d]*step;
            if(!pos_ok(tx,ty))break;
            Tile tile=map[ty][tx];
            if(!walkable(tile)&&tile!=TILE_BRICK)break;
            result.flame[ty][tx]=true;result.flame_count++;
            if(tile==TILE_BRICK){
                result.destroy_brick[ty][tx]=true;result.brick_count++;
                break;
            }
        }
    }
    return result;
}
bool bm_flame_hit(const bool flame[MAP_H][MAP_W],float x,float y,bool alive){
    if(!alive||!isfinite(x)||!isfinite(y))return false;
    const double radius=BODY_RADIUS;
    for(int ty=0;ty<MAP_H;ty++)for(int tx=0;tx<MAP_W;tx++)if(flame[ty][tx]){
        double nearest_x=fmax(tx-0.5,fmin((double)x,tx+0.5));
        double nearest_y=fmax(ty-0.5,fmin((double)y,ty+0.5));
        double dx=(double)x-nearest_x,dy=(double)y-nearest_y;
        if(dx*dx+dy*dy<radius*radius)return true;
    }
    return false;
}
int bm_score_delta(BmScoreEvent event){
    switch(event){
        case BM_SCORE_KILL:return 10;
        case BM_SCORE_CONTACT:return -10;
        case BM_SCORE_BOMB:return -20;
        default:return 0;
    }
}
BmDamageResult bm_apply_damage(BmDamageInput in){
    BmDamageResult result={.valid=false,.applied=false,.lives=in.lives,
                           .score=in.score,.stun=in.stun,.phase=in.phase};
    if(in.lives<0||in.lives>99||in.score<-1000000||in.score>1000000||
       !isfinite(in.stun)||in.stun<0||!isfinite(in.grace)||in.grace<0||
       in.phase<PLAYING||in.phase>ALL_CLEAR||in.cause<BM_SCORE_KILL||in.cause>BM_SCORE_BOMB)return result;
    result.valid=true;
    if(in.phase!=PLAYING||in.lives==0||in.stun>BM_EPSILON||in.grace>BM_EPSILON)return result;
    result.applied=true;result.lives--;result.score+=bm_score_delta(in.cause);result.stun=STUN_TIME;
    if(result.lives==0)result.phase=GAME_OVER;
    return result;
}
BmExitResult bm_exit_request(int level,Tile tile,bool moving,float stun,Phase phase){
    BmExitResult result={.status=BM_EXIT_INVALID,.next_level=level,.phase=phase,.flash=0,.reset_map=false};
    if(level<1||level>3||!isfinite(stun)||stun<0||phase<PLAYING||phase>ALL_CLEAR)return result;
    result.status=BM_EXIT_BLOCKED;
    if(tile!=TILE_EXIT||moving||stun>BM_EPSILON||phase!=PLAYING)return result;
    if(level<3){
        result.status=BM_EXIT_ADVANCE;result.next_level=level+1;result.phase=LEVEL_FLASH;
        result.flash=LEVEL_FLASH_TIME;result.reset_map=true;
    }else{
        result.status=BM_EXIT_COMPLETE;result.phase=ALL_CLEAR;
    }
    return result;
}
BmStageStart bm_stage_start(bool new_game,int level,int lives,int score){
    /* A new game ignores the supplied continuation state. */
    if(new_game){
        return (BmStageStart){.valid=true,.level=1,.lives=START_LIVES,
                              .score=0,.time_left=START_TIME};
    }
    if(level<1||level>3||lives<1||lives>99||score<-1000000||score>1000000){
        return (BmStageStart){0};
    }
    return (BmStageStart){.valid=true,.level=level,.lives=lives,
                          .score=score,.time_left=START_TIME};
}
BmFlashResult bm_level_flash(float remaining,float dt){
    BmFlashResult result={.valid=false,.white=false,.remaining=0,.phase=PLAYING};
    if(!isfinite(remaining)||remaining<0||remaining>LEVEL_FLASH_TIME||!isfinite(dt)||dt<0)return result;
    result.valid=true;result.remaining=bm_tick_timer(remaining,dt).remaining;
    if(result.remaining>0){
        result.phase=LEVEL_FLASH;
        int interval=(int)floorf((LEVEL_FLASH_TIME-result.remaining)/FLASH_BLINK);
        result.white=interval%2==0;
    }
    return result;
}
