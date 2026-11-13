#include "rules.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

int main(void){
    int checks=0,failures=0;
    bool open[4],before[4];
    const Direction currents[]={DIR_DOWN,DIR_RIGHT,DIR_UP,DIR_LEFT,DIR_NONE,
                                 (Direction)-1,(Direction)INT_MIN,(Direction)INT_MAX};
    /* Explicit expected directions for masks 0..15; bits are DOWN/RIGHT/UP/LEFT. */
    const Direction down_first[16]={DIR_NONE,DIR_DOWN,DIR_NONE,DIR_DOWN,
        DIR_UP,DIR_DOWN,DIR_UP,DIR_DOWN,DIR_NONE,DIR_DOWN,DIR_NONE,DIR_DOWN,
        DIR_UP,DIR_DOWN,DIR_UP,DIR_DOWN};
    const Direction up_first[16]={DIR_NONE,DIR_DOWN,DIR_NONE,DIR_DOWN,
        DIR_UP,DIR_UP,DIR_UP,DIR_UP,DIR_NONE,DIR_DOWN,DIR_NONE,DIR_DOWN,
        DIR_UP,DIR_UP,DIR_UP,DIR_UP};
    const Direction left_first[16]={DIR_NONE,DIR_NONE,DIR_RIGHT,DIR_RIGHT,
        DIR_NONE,DIR_NONE,DIR_RIGHT,DIR_RIGHT,DIR_LEFT,DIR_LEFT,DIR_LEFT,DIR_LEFT,
        DIR_LEFT,DIR_LEFT,DIR_LEFT,DIR_LEFT};
    const Direction right_first[16]={DIR_NONE,DIR_NONE,DIR_RIGHT,DIR_RIGHT,
        DIR_NONE,DIR_NONE,DIR_RIGHT,DIR_RIGHT,DIR_LEFT,DIR_LEFT,DIR_RIGHT,DIR_RIGHT,
        DIR_LEFT,DIR_LEFT,DIR_RIGHT,DIR_RIGHT};
    for(unsigned mask=0;mask<16;mask++){
        for(int d=0;d<4;d++)open[d]=(mask&(1u<<d))!=0;
        memcpy(before,open,sizeof open);
        for(size_t c=0;c<sizeof currents/sizeof currents[0];c++){
            Direction white=currents[c]==DIR_UP?up_first[mask]:down_first[mask];
            Direction yellow=currents[c]==DIR_RIGHT?right_first[mask]:left_first[mask];
            CHECK(bm_monster_direction(MONSTER_WHITE,currents[c],open,0)==white);
            CHECK(bm_monster_direction(MONSTER_WHITE,currents[c],open,UINT32_MAX)==white);
            CHECK(bm_monster_direction(MONSTER_YELLOW,currents[c],open,0)==yellow);
            CHECK(bm_monster_direction(MONSTER_YELLOW,currents[c],open,UINT32_MAX)==yellow);
        }
        CHECK(memcmp(before,open,sizeof open)==0);
    }

    /* Green heading DOWN: each row states the expected repeating choice cycle. */
    const struct {int count;Direction choices[3];} green[16]={
        {0,{DIR_NONE}}, {1,{DIR_DOWN}}, {1,{DIR_RIGHT}}, {1,{DIR_DOWN}},
        {1,{DIR_UP}}, {1,{DIR_DOWN}}, {1,{DIR_UP}}, {2,{DIR_DOWN,DIR_RIGHT}},
        {1,{DIR_LEFT}}, {1,{DIR_DOWN}}, {2,{DIR_RIGHT,DIR_LEFT}},
        {3,{DIR_DOWN,DIR_RIGHT,DIR_LEFT}}, {1,{DIR_UP}}, {2,{DIR_DOWN,DIR_LEFT}},
        {2,{DIR_RIGHT,DIR_LEFT}}, {3,{DIR_DOWN,DIR_RIGHT,DIR_LEFT}}
    };
    const uint32_t samples[]={0,1,2,3,4,UINT32_MAX,UINT32_C(0x80000000)};
    for(int rotation=0;rotation<4;rotation++)for(unsigned mask=0;mask<16;mask++){
        for(int d=0;d<4;d++)open[(d+rotation)%4]=(mask&(1u<<d))!=0;
        memcpy(before,open,sizeof open);
        Direction choices[3];
        for(int j=0;j<green[mask].count;j++)choices[j]=(Direction)((green[mask].choices[j]+rotation)%4);
        /* Rotation changes numeric ordering; restore the specified global order. */
        for(int j=0;j<green[mask].count;j++)for(int k=j+1;k<green[mask].count;k++){
            if(choices[k]<choices[j]){Direction tmp=choices[j];choices[j]=choices[k];choices[k]=tmp;}
        }
        for(size_t s=0;s<sizeof samples/sizeof samples[0];s++){
            Direction expected=green[mask].count?choices[samples[s]%(uint32_t)green[mask].count]:DIR_NONE;
            CHECK(bm_monster_direction(MONSTER_GREEN,(Direction)rotation,open,samples[s])==expected);
            CHECK(memcmp(before,open,sizeof open)==0);
        }
    }

    /* Invalid green headings use the unrotated DOWN table. */
    for(unsigned mask=0;mask<16;mask++){
        for(int d=0;d<4;d++)open[d]=(mask&(1u<<d))!=0;
        for(size_t c=4;c<sizeof currents/sizeof currents[0];c++){
            for(size_t s=0;s<sizeof samples/sizeof samples[0];s++){
                Direction expected=green[mask].count?green[mask].choices[samples[s]%(uint32_t)green[mask].count]:DIR_NONE;
                CHECK(bm_monster_direction(MONSTER_GREEN,currents[c],open,samples[s])==expected);
            }
        }
        CHECK(bm_monster_direction((MonsterKind)-1,DIR_DOWN,open,0)==DIR_NONE);
        CHECK(bm_monster_direction((MonsterKind)INT_MIN,DIR_DOWN,open,0)==DIR_NONE);
        CHECK(bm_monster_direction((MonsterKind)INT_MAX,DIR_DOWN,open,UINT32_MAX)==DIR_NONE);
    }
    printf("Monster direction: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
