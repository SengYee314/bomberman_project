#ifndef BOMBER_GAME_H
#define BOMBER_GAME_H
#include <stdbool.h>
#include <stdint.h>
#define MAP_W 15
#define MAP_H 13
#define MAX_MONSTERS 3
#define MAX_BOMBS 16
#define FIXED_DT (1.0f/120.0f)
#define START_TIME 180.0f
#define START_LIVES 3
#define BOMB_FUSE 3.0f
#define BOMB_COOLDOWN 5.0f
#define STUN_TIME 5.0f
#define FLAME_TIME 0.65f
#define LEVEL_FLASH_TIME 0.9f
#define FLASH_BLINK 0.15f
#define BODY_RADIUS 0.22f
#define PORTAL_COOLDOWN 0.75f
#define DEMOLITION_TARGET 3
#define DEMOLITION_WINDOW_TICKS 1800 /* 15 seconds at FIXED_DT = 1/120. */
#define DEMOLITION_BONUS 30
#define DEMOLITION_NOTICE_TIME 2.0f

typedef enum { TILE_FLOOR, TILE_WALL, TILE_BRICK, TILE_EXIT, TILE_PORTAL_A, TILE_PORTAL_B } Tile;
typedef enum { DIR_DOWN, DIR_RIGHT, DIR_UP, DIR_LEFT, DIR_NONE } Direction;
typedef enum { MONSTER_WHITE, MONSTER_YELLOW, MONSTER_GREEN } MonsterKind;
typedef enum { PLAYING, LEVEL_FLASH, GAME_OVER, ALL_CLEAR } Phase;
typedef struct {
    /* x,y: 타일 중심 좌표. cx,cy: 출발 타일, tx,ty: 도착 타일. */
    float x,y,anim;
    int cx,cy,tx,ty;
    Direction dir;
    bool moving;
} Actor;
typedef struct { Actor body; MonsterKind kind; bool alive; } Monster;
typedef struct { int x,y; float fuse; bool active; } Bomb;
typedef struct { Direction wanted; bool bomb_pressed; } Input;
typedef struct {
    Tile map[MAP_H][MAP_W];
    float flame[MAP_H][MAP_W],crumble[MAP_H][MAP_W];
    Actor player;
    Monster monsters[MAX_MONSTERS];
    Bomb bombs[MAX_BOMBS];
    int level,lives,score;
    float time_left,cooldown,stun,grace,flash,portal_cooldown;
    bool portals_enabled,forecast_enabled;
    bool portal_latched; /* Leave a portal tile before teleporting again. */
    bool demolition_enabled;
    int demolition_bricks,demolition_ticks_left,demolition_pending;
    float demolition_notice;
    bool stage_invalid; /* bm_stage_start가 valid=false: main.c는 진행을 멈추고 renderer는 구현 안내를 표시 */
    Phase phase;
    uint32_t rng;
} Game;
void game_init(Game *g, uint32_t seed, bool extras);
void game_tick(Game *g, Input in); /* 반드시 FIXED_DT마다 호출: 렌더 FPS와 분리 */
bool game_can_enter(const Game *g, int x,int y);
float game_player_speed(const Game *g);
float game_monster_speed(const Game *g);
int game_blast_range(const Game *g);
void game_move_player(Game *g, Direction wanted);
void game_move_monsters(Game *g);
bool game_place_bomb(Game *g);
void game_update_bombs(Game *g);
void game_hit_player(Game *g,bool by_bomb);
void game_check_collisions(Game *g);
void game_check_exit(Game *g);
void game_try_portal(Game *g);
/* danger[y][x]: 현재 설치된 폭탄의 정적 위험 지도. route는 가까운 안전 타일까지 BFS 경로. */
int game_forecast(const Game *g,bool danger[MAP_H][MAP_W],int route[MAP_W*MAP_H][2]);
#endif
