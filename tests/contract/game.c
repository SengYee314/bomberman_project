#include "game.h"
#include "rules.h"
#include <math.h>
#include <string.h>

static const int DX[4]={0,1,0,-1},DY[4]={1,0,-1,0};
static const char *LEVEL_MAP[MAP_H]={
 "###############",
 "#P..b....b...W#",
 "#.#.#.#.#.#.#.#",
 "#..b...b...b..#",
 "#.#.#.#.#.#.#.#",
 "#b...Y...b....#",
 "#.#.#.#.#.#.#.#",
 "#..b..b..b..G.#",
 "#.#.#.#.#.#.#.#",
 "#..b......b...#",
 "#.#.#.#.#.#.#.#",
 "#..b....b....D#",
 "###############"
};
static bool inside(int x,int y){return x>=0&&y>=0&&x<MAP_W&&y<MAP_H;}
static int cell(float p){return (int)floorf(p+0.5f);}
static uint32_t random_u32(Game *g){g->rng^=g->rng<<13;g->rng^=g->rng>>17;g->rng^=g->rng<<5;return g->rng;}
static void actor_init(Actor *a,int x,int y,Direction d){*a=(Actor){.x=(float)x,.y=(float)y,.cx=x,.cy=y,.tx=x,.ty=y,.dir=d};}
static bool bomb_at(const Game *g,int x,int y){for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active&&g->bombs[i].x==x&&g->bombs[i].y==y)return true;return false;}
/* 스테이지 시작 상태(레벨·목숨·점수·시간)는 학생 함수 bm_stage_start의 결과를 그대로 반영한다. */
static void setup_level(Game *g,BmStageStart start){
    memset(g->bombs,0,sizeof g->bombs);memset(g->flame,0,sizeof g->flame);memset(g->crumble,0,sizeof g->crumble);
    g->level=start.level;g->lives=start.lives;g->score=start.score;g->time_left=start.time_left;g->stage_invalid=!start.valid;
    int n=0;
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
        char c=LEVEL_MAP[y][x];g->map[y][x]=c=='#'?TILE_WALL:c=='b'?TILE_BRICK:c=='D'?TILE_EXIT:TILE_FLOOR;
        if(c=='P')actor_init(&g->player,x,y,DIR_DOWN);
        if(c=='W'||c=='Y'||c=='G'){
            Monster *m=&g->monsters[n++];m->alive=true;m->kind=c=='W'?MONSTER_WHITE:c=='Y'?MONSTER_YELLOW:MONSTER_GREEN;
            actor_init(&m->body,x,y,m->kind==MONSTER_YELLOW?DIR_LEFT:DIR_DOWN);
        }
    }
    if(g->portals_enabled){g->map[3][5]=TILE_PORTAL_A;g->map[9][9]=TILE_PORTAL_B;}
    g->cooldown=0;g->stun=0;g->grace=1;g->portal_cooldown=0;
}
void game_init(Game *g,uint32_t seed,bool extras){
    memset(g,0,sizeof *g);g->rng=seed?seed:1;g->phase=PLAYING;
    (void)extras;g->portals_enabled=false;g->forecast_enabled=false;
    setup_level(g,bm_stage_start(true,0,0,0));
}
float game_player_speed(const Game *g){
    return bm_level_rules(g->level).player_speed;
}

float game_monster_speed(const Game *g){
    return bm_level_rules(g->level).monster_speed;
}

int game_blast_range(const Game *g){
    return bm_level_rules(g->level).blast_range;
}

bool game_can_enter(const Game *g,int x,int y){
    bool occupied[MAP_H][MAP_W]={{false}};
    for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active)occupied[g->bombs[i].y][g->bombs[i].x]=true;
    return bm_can_enter(BM_MAP_VIEW(g->map),BM_BOMB_VIEW(occupied),x,y);
}

static bool start_move(Game *g,Actor *a,Direction d){
    if(d==DIR_NONE||!game_can_enter(g,a->cx+DX[d],a->cy+DY[d]))return false;
    a->tx=a->cx+DX[d];a->ty=a->cy+DY[d];a->dir=d;a->moving=true;return true;
}
static void advance(Actor *a,float speed){
    if(!a->moving)return;
    const float step=speed*FIXED_DT,dist=fabsf(a->tx-a->x)+fabsf(a->ty-a->y);
    if(step>=dist){a->x=(float)a->tx;a->y=(float)a->ty;a->cx=a->tx;a->cy=a->ty;a->moving=false;}
    else{a->x+=DX[a->dir]*step;a->y+=DY[a->dir]*step;}
    a->anim+=FIXED_DT*8;
}
void game_move_player(Game *g,Direction wanted){
    Actor *p=&g->player;if(g->stun>0)return;
    bool occupied[MAP_H][MAP_W]={{false}};
    for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active)occupied[g->bombs[i].y][g->bombs[i].x]=true;
    BmMoveDecision next=bm_player_step(BM_MAP_VIEW(g->map),BM_BOMB_VIEW(occupied),p->cx,p->cy,p->dir,wanted,p->moving,g->stun>0);
    if(next.start)start_move(g,p,next.direction);
    advance(p,game_player_speed(g));
}

void game_move_monsters(Game *g){
    for(int i=0;i<MAX_MONSTERS;i++){
        Monster *m=&g->monsters[i];Actor *a=&m->body;if(!m->alive)continue;
        if(!a->moving){bool open[4];for(int d=0;d<4;d++)open[d]=game_can_enter(g,a->cx+DX[d],a->cy+DY[d]);
            Direction next=bm_monster_direction(m->kind,a->dir,open,m->kind==MONSTER_GREEN?random_u32(g):0);
            start_move(g,a,next);
        }
        advance(a,game_monster_speed(g));
    }
}

bool game_place_bomb(Game *g){
    int x=cell(g->player.x),y=cell(g->player.y),slots=0;
    for(int i=0;i<MAX_BOMBS;i++)slots+=!g->bombs[i].active;
    BmBombDecision next=bm_bomb_request((BmBombRequest){g->phase==PLAYING,g->stun>0,bomb_at(g,x,y),slots,g->cooldown});
    if(!next.allowed)return false;
    for(int i=0;i<MAX_BOMBS;i++)if(!g->bombs[i].active){g->bombs[i]=(Bomb){x,y,next.fuse,true};g->cooldown=next.cooldown;return true;}
    return false;
}

static void add_flame(Game *g,int x,int y){
    g->flame[y][x]=FLAME_TIME;
    for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active&&g->bombs[i].x==x&&g->bombs[i].y==y)g->bombs[i].fuse=0;
}
static void detonate(Game *g,int index){
    Bomb b=g->bombs[index];g->bombs[index].active=false;
    BmBlastResult blast=bm_trace_blast(BM_MAP_VIEW(g->map),b.x,b.y,game_blast_range(g));
    if(!blast.valid)return;
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)if(blast.flame[y][x]){
        add_flame(g,x,y);
        if(blast.destroy_brick[y][x]){g->map[y][x]=TILE_FLOOR;g->crumble[y][x]=.4f;}
    }
}

void game_update_bombs(Game *g){
    for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active)g->bombs[i].fuse=bm_tick_timer(g->bombs[i].fuse,FIXED_DT).remaining;
    /* 뒤쪽 슬롯이 앞쪽 폭탄을 연쇄 점화할 때도 같은 tick 안에 처리한다. */
    for(int pass=0;pass<MAX_BOMBS;pass++){
        bool any=false;
        for(int i=0;i<MAX_BOMBS;i++)if(g->bombs[i].active&&g->bombs[i].fuse<=0.00001f){detonate(g,i);any=true;}
        if(!any)break;
    }
}
void game_hit_player(Game *g,bool by_bomb){
    BmDamageResult next=bm_apply_damage((BmDamageInput){g->lives,g->score,g->stun,g->grace,g->phase,by_bomb?BM_SCORE_BOMB:BM_SCORE_CONTACT});
    if(!next.valid||!next.applied)return;
    g->lives=next.lives;g->score=next.score;g->stun=next.stun;g->phase=next.phase;g->player.moving=false;
}

void game_check_collisions(Game *g){
    bool burning[MAP_H][MAP_W];
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++)burning[y][x]=g->flame[y][x]>0;
    for(int i=0;i<MAX_MONSTERS;i++){Monster *m=&g->monsters[i];if(bm_flame_hit(BM_FLAME_VIEW(burning),m->body.x,m->body.y,m->alive)){m->alive=false;g->score+=bm_score_delta(BM_SCORE_KILL);}}
    if(bm_flame_hit(BM_FLAME_VIEW(burning),g->player.x,g->player.y,true)){game_hit_player(g,true);return;} /* 동시 충돌은 폭탄 우선, 한 번만 감점 */
    for(int i=0;i<MAX_MONSTERS;i++)if(g->monsters[i].alive){
        Actor *m=&g->monsters[i].body;float dx=m->x-g->player.x,dy=m->y-g->player.y;
        if(dx*dx+dy*dy<.52f*.52f){game_hit_player(g,false);break;}
    }
}
void game_check_exit(Game *g){
    BmExitResult next=bm_exit_request(g->level,g->map[g->player.cy][g->player.cx],g->player.moving,g->stun,g->phase);
    if(next.status!=BM_EXIT_ADVANCE&&next.status!=BM_EXIT_COMPLETE)return;
    if(next.reset_map)setup_level(g,bm_stage_start(false,next.next_level,g->lives,g->score));
    else g->level=next.next_level;
    g->phase=next.phase;g->flash=next.flash;
}

void game_try_portal(Game *g){
    /* 학생 추가 기능 공간 */
    (void)g;
}

int game_forecast(const Game *g,bool danger[MAP_H][MAP_W],int route[MAP_W*MAP_H][2]){
    /* 학생 추가 기능 공간 */
    memset(danger,0,sizeof(bool)*MAP_W*MAP_H);(void)g;(void)route;return 0;
}

void game_tick(Game *g,Input in){
    if(g->phase==GAME_OVER||g->phase==ALL_CLEAR)return;
    if(g->phase==LEVEL_FLASH){BmFlashResult next=bm_level_flash(g->flash,FIXED_DT);g->flash=next.remaining;g->phase=next.phase;return;}
    g->time_left-=FIXED_DT;if(g->time_left<=0){g->time_left=0;g->phase=GAME_OVER;return;}
    g->cooldown=bm_tick_timer(g->cooldown,FIXED_DT).remaining;g->grace=fmaxf(0,g->grace-FIXED_DT);g->portal_cooldown=fmaxf(0,g->portal_cooldown-FIXED_DT);
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){g->flame[y][x]=fmaxf(0,g->flame[y][x]-FIXED_DT);g->crumble[y][x]=fmaxf(0,g->crumble[y][x]-FIXED_DT);}
    if(g->stun>0){BmTimerResult next=bm_tick_timer(g->stun,FIXED_DT);g->stun=next.remaining;if(next.just_expired){actor_init(&g->player,1,1,DIR_DOWN);g->grace=1;}}
    game_move_player(g,in.wanted);game_move_monsters(g);
    if(in.bomb_pressed)game_place_bomb(g);
    game_update_bombs(g);game_check_collisions(g);
    if(g->phase==PLAYING&&g->stun<=0){game_try_portal(g);game_check_exit(g);}
}
