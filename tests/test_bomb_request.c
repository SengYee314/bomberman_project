#include "rules.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

static bool same_bomb(BmBombDecision got,BmBombDecision expected){
    return got.allowed==expected.allowed&&got.status==expected.status&&
           got.fuse==expected.fuse&&got.cooldown==expected.cooldown;
}

int main(void){
    int checks=0,failures=0;
    /* Rows: flags ACTIVE=1, STUNNED=2, DUPLICATE=4.
       Columns: available, tiny cooldown, full, full with cooldown. */
    const BmPlaceStatus expected_status[8][4]={
        {BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE},
        {BM_PLACE_OK,BM_PLACE_COOLDOWN,BM_PLACE_FULL,BM_PLACE_COOLDOWN},
        {BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE},
        {BM_PLACE_STUNNED,BM_PLACE_STUNNED,BM_PLACE_STUNNED,BM_PLACE_STUNNED},
        {BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE},
        {BM_PLACE_DUPLICATE,BM_PLACE_COOLDOWN,BM_PLACE_DUPLICATE,BM_PLACE_COOLDOWN},
        {BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE,BM_PLACE_INACTIVE},
        {BM_PLACE_STUNNED,BM_PLACE_STUNNED,BM_PLACE_STUNNED,BM_PLACE_STUNNED}
    };
    const int slots[4]={1,1,0,0};
    const float cooldowns[4]={0,BM_EPSILON/2,0,2};
    for(int flags=0;flags<8;flags++)for(int mode=0;mode<4;mode++){
        BmBombRequest request={(flags&1)!=0,(flags&2)!=0,(flags&4)!=0,slots[mode],cooldowns[mode]};
        BmPlaceStatus status=expected_status[flags][mode];
        BmBombDecision expected={status==BM_PLACE_OK,status,status==BM_PLACE_OK?3.0f:0.0f,
                                  status==BM_PLACE_OK?5.0f:cooldowns[mode]};
        CHECK(same_bomb(bm_bomb_request(request),expected));
    }

    /* Every valid positive capacity works; maximum capacity is inclusive. */
    for(int free_slots=1;free_slots<=MAX_BOMBS;free_slots++){
        CHECK(same_bomb(bm_bomb_request((BmBombRequest){true,false,false,free_slots,0}),
                        (BmBombDecision){true,BM_PLACE_OK,3,5}));
    }
    const float invalid_cooldowns[]={-1,-BM_EPSILON,NAN,INFINITY,-INFINITY};
    for(size_t i=0;i<sizeof invalid_cooldowns/sizeof invalid_cooldowns[0];i++){
        for(int flags=0;flags<8;flags++){
            CHECK(same_bomb(bm_bomb_request((BmBombRequest){(flags&1)!=0,(flags&2)!=0,(flags&4)!=0,0,invalid_cooldowns[i]}),
                            (BmBombDecision){false,BM_PLACE_INVALID,0,0}));
        }
    }
    const int invalid_slots[]={-1,MAX_BOMBS+1,INT_MIN,INT_MAX};
    const float retained[]={0,0.25f,FLT_MAX};
    for(size_t i=0;i<sizeof invalid_slots/sizeof invalid_slots[0];i++){
        for(size_t j=0;j<sizeof retained/sizeof retained[0];j++){
            for(int flags=0;flags<8;flags++){
                CHECK(same_bomb(bm_bomb_request((BmBombRequest){(flags&1)!=0,(flags&2)!=0,(flags&4)!=0,invalid_slots[i],retained[j]}),
                                (BmBombDecision){false,BM_PLACE_INVALID,0,retained[j]}));
            }
        }
        CHECK(same_bomb(bm_bomb_request((BmBombRequest){false,true,true,invalid_slots[i],NAN}),
                        (BmBombDecision){false,BM_PLACE_INVALID,0,0}));
    }
    CHECK(same_bomb(bm_bomb_request((BmBombRequest){true,false,false,1,nextafterf(0.0f,1.0f)}),
                    (BmBombDecision){false,BM_PLACE_COOLDOWN,0,nextafterf(0.0f,1.0f)}));
    CHECK(same_bomb(bm_bomb_request((BmBombRequest){true,false,false,1,FLT_MAX}),
                    (BmBombDecision){false,BM_PLACE_COOLDOWN,0,FLT_MAX}));
    CHECK(same_bomb(bm_bomb_request((BmBombRequest){true,false,false,1,-0.0f}),
                    (BmBombDecision){true,BM_PLACE_OK,3,5}));
    printf("Bomb request: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
