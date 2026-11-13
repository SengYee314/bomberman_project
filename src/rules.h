#ifndef BOMBER_RULES_H
#define BOMBER_RULES_H
#include "game.h"
/* 이 헤더와 자료구조는 변경하지 않습니다.
   순수 함수: 입력을 변경하지 않고 결과를 반환합니다. 난수 표본/경과시간도 입력받습니다.
   전역 Game, 현재 시각, 파일/콘솔 I/O, Sleep에 의존하지 않습니다.
   map/bombs는 NULL이 아닌 MAP_H x MAP_W 배열. float 비교 허용오차는 1e-4입니다. */
#define BM_RULES_API_VERSION 4
#define BM_MAP_VIEW(map) ((const Tile (*)[MAP_W])(map))
#define BM_BOMB_VIEW(bombs) ((const bool (*)[MAP_W])(bombs))
#define BM_FLAME_VIEW(flame) ((const bool (*)[MAP_W])(flame))
#define BM_EPSILON 0.00001f

typedef struct { bool start; Direction direction; int target_x,target_y; } BmMoveDecision;
typedef struct { bool valid; float player_speed,monster_speed; int blast_range; } BmLevelRules;
typedef enum { BM_PLACE_OK, BM_PLACE_INVALID, BM_PLACE_INACTIVE, BM_PLACE_STUNNED,
               BM_PLACE_COOLDOWN, BM_PLACE_DUPLICATE, BM_PLACE_FULL } BmPlaceStatus;
typedef struct { bool game_active,stunned,tile_has_bomb; int free_slots; float cooldown; } BmBombRequest;
typedef struct { bool allowed; BmPlaceStatus status; float fuse,cooldown; } BmBombDecision;
typedef struct { bool valid,just_expired; float remaining; } BmTimerResult;
typedef struct { bool valid; bool flame[MAP_H][MAP_W],destroy_brick[MAP_H][MAP_W]; int flame_count,brick_count; } BmBlastResult;
typedef enum { BM_SCORE_KILL, BM_SCORE_CONTACT, BM_SCORE_BOMB } BmScoreEvent;
typedef struct { int lives,score; float stun,grace; Phase phase; BmScoreEvent cause; } BmDamageInput;
typedef struct { bool valid,applied; int lives,score; float stun; Phase phase; } BmDamageResult;
typedef enum { BM_EXIT_INVALID, BM_EXIT_BLOCKED, BM_EXIT_ADVANCE, BM_EXIT_COMPLETE } BmExitStatus;
typedef struct { BmExitStatus status; int next_level; Phase phase; float flash; bool reset_map; } BmExitResult;
typedef struct { bool valid; int level,lives,score; float time_left; } BmStageStart;
typedef struct { bool valid,white; float remaining; Phase phase; } BmFlashResult;

/* 범위 밖/벽/벽돌/폭탄/알 수 없는 타일=false. 바닥/출구/포탈=true. 입력 불변. */
bool bm_can_enter(const Tile map[MAP_H][MAP_W],const bool bombs[MAP_H][MAP_W],int x,int y);
/* moving/stunned/요청 NONE이면 start=false, 현 좌표,방향 유지.
   중심에서 요청 방향이 막히면 기존 facing 방향 시도. 둘 다 막히면 정지.
   현재 좌표가 범위 밖이거나 요청 방향이 잘못되어도 현 좌표·방향을 유지하며 start=false. */
BmMoveDecision bm_player_step(const Tile map[MAP_H][MAP_W],const bool bombs[MAP_H][MAP_W],
                             int x,int y,Direction facing,Direction requested,bool moving,bool stunned);
/* 하양: 상하만, 잘못된 초기 방향은 DOWN. 노랑: 좌우만, 잘못된 초기 방향은 LEFT.
   초록: 현재 길 유지, 막히면 반대. 3갈래 이상에서는 역방향 제외한 DOWN/RIGHT/UP/LEFT
   순서의 합법 후보 중 sample % 후보수 선택. 현재·반대 방향이 모두 막히면(옆으로만 열린 칸)
   열린 방향을 같은 순서로 모아 sample % 후보수 선택. 길이 없으면 NONE. 잘못된 kind는 NONE, 초록의 잘못된 current는 DOWN으로 간주.
   입력 open은 변경 금지. */
Direction bm_monster_direction(MonsterKind kind,Direction current,const bool open[4],uint32_t sample);
/* level 1/2/3 -> Player 3/3.9/4.8, Monster 1.6/1.92/2.4, 범위 2/3/4.
   그 외 -> valid=false 및 나머지 필드 0. */
BmLevelRules bm_level_rules(int level);
/* 유효 입력: free_slots 0..MAX_BOMBS, finite cooldown>=0.
   거절 우선순위: INVALID, INACTIVE, STUNNED, COOLDOWN, DUPLICATE, FULL.
   성공 -> OK, fuse=3, cooldown=5. 거절 -> fuse=0, cooldown=입력 cooldown(잘못된 값이면0). */
BmBombDecision bm_bomb_request(BmBombRequest request);
/* finite remaining>=0, finite dt>=0. max(0,remaining-dt), EPS 이하를 0으로 정규화.
   just_expired는 양수였던 타이머가 이번 호출에서 0이 될 때만 true. 잘못된 입력은 전부 0/false. */
BmTimerResult bm_tick_timer(float remaining,float dt);
/* range=1..4, 시작점은 바닥/출구/포탈이어야 함. 중심+4방향, 벽 제외/차단,
   벽돌 한 개 포함·destroy_brick 표시 후 그 방향 종료. map 변경 금지.
   알 수 없는 타일도 벽처럼 제외·차단. invalid -> valid=false, 모든 칸 false, count=0. */
BmBlastResult bm_trace_blast(const Tile map[MAP_H][MAP_W],int x,int y,int range);
/* 살아 있는 캐릭터의 몸체(중심 x,y는 타일 좌표, 반경 BODY_RADIUS=0.22)가 flame이 true인 타일과 겹치면 true.
   타일 (tx,ty)는 중심 (tx,ty), 한 변 1인 정사각형 [tx-0.5,tx+0.5]x[ty-0.5,ty+0.5].
   사각형 안에서 몸체 중심에 가장 가까운 점까지의 거리가 반경보다 작으면 겹침(같으면 겹치지 않음).
   alive=false 또는 x/y가 유한하지 않으면 false. 입력 flame 불변.
   엔진은 몬스터 사망(+10은 점수 시스템)과 Player 폭탄 피격 판정에 이 함수를 사용합니다. */
bool bm_flame_hit(const bool flame[MAP_H][MAP_W],float x,float y,bool alive);
/* KILL +10, CONTACT -10, BOMB -20, 그 외 0. */
int bm_score_delta(BmScoreEvent event);
/* 입력 lives 0..99, score -1000000..1000000, finite stun/grace>=0, 유효 phase/cause.
   PLAYING & lives>0 & stun/grace<=EPS일 때만 적용. 1목숨 차감, 감점, stun=5.
   남은 목숨0이면 GAME_OVER. 무시된 피격은 입력 상태 유지. invalid도 상태유지, valid=false. */
BmDamageResult bm_apply_damage(BmDamageInput input);
/* level1..3 & PLAYING & EXIT & !moving & stun<=EPS일 때 승급.
   1/2 -> ADVANCE, level+1, LEVEL_FLASH, flash=LEVEL_FLASH_TIME(0.9), reset_map=true.
   3 -> COMPLETE, level3, ALL_CLEAR, flash=0, reset_map=false. 나머지는 상태유지.
   잘못된 level/stun/phase는 INVALID. 미승급 시 flash=0, reset_map=false. */
BmExitResult bm_exit_request(int level,Tile tile,bool moving,float stun,Phase phase);
/* 스테이지 시작 상태. 엔진은 새 게임과 다음 레벨 맵 복원 때 호출합니다.
   new_game=true -> level=1, lives=START_LIVES(3), score=0, time_left=START_TIME(180). 나머지 인자는 무시.
   new_game=false(다음 레벨) -> level 1..3, lives 1..99, score -1000000..1000000이면
   level·lives·score를 그대로 유지하고 time_left=START_TIME. 그 외 -> valid=false, 나머지 0. */
BmStageStart bm_stage_start(bool new_game,int level,int lives,int score);
/* 레벨업 점멸 한 단계. 유효 입력: finite 0<=remaining<=LEVEL_FLASH_TIME, finite dt>=0.
   새 remaining은 bm_tick_timer와 같은 규칙(max(0,remaining-dt), EPS 이하는 0).
   새 remaining>0 -> phase=LEVEL_FLASH. 경과=LEVEL_FLASH_TIME-새 remaining을 FLASH_BLINK(0.15)로 나눈
   몫(소수 버림)이 짝수면 white=true(맵을 흰색으로), 홀수면 false. 0.9초 동안 흰색 3번.
   새 remaining==0 -> phase=PLAYING, white=false. 잘못된 입력 -> valid=false, white=false, remaining=0, phase=PLAYING.
   구간 경계 ±1e-4는 채점하지 않습니다. 렌더러는 dt=0으로 호출해 현재 white를 얻습니다. */
BmFlashResult bm_level_flash(float remaining,float dt);
#endif
