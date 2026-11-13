#include "rules.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do{checks++;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);failures++;}}while(0)

int main(void){
    int checks=0,failures=0;
    bool flame[MAP_H][MAP_W]={{false}},before[MAP_H][MAP_W];
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),6,5,true));
    const struct {float dx,dy;bool hit;} points[]={
        {0,0,true},{0.5f,0,true},{-0.5f,0,true},{0,0.5f,true},{0,-0.5f,true},
        {0.7f,0,true},{-0.7f,0,true},{0,0.7f,true},{0,-0.7f,true},
        {0.75f,0,false},{-0.75f,0,false},{0,0.75f,false},{0,-0.75f,false},
        {0.65f,0.65f,true},{-0.65f,-0.65f,true},{0.66f,0.66f,false},
        {0.72f,0.72f,false},{1,0,false},{0,1,false}
    };
    for(int y=0;y<MAP_H;y++)for(int x=0;x<MAP_W;x++){
        flame[y][x]=true;memcpy(before,flame,sizeof flame);
        for(size_t i=0;i<sizeof points/sizeof points[0];i++){
            CHECK(bm_flame_hit(BM_FLAME_VIEW(flame),x+points[i].dx,y+points[i].dy,true)==points[i].hit);
        }
        CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),(float)x,(float)y,false));
        CHECK(memcmp(before,flame,sizeof flame)==0);
        flame[y][x]=false;
    }
    /* Exactly representable 3-4-5 tangent to the lower-left corner of tile (1,1). */
    flame[1][1]=true;
    double radius=BODY_RADIUS;
    float tangent_x=(float)(0.5-radius*3/5),tangent_y=(float)(0.5-radius*4/5);
    double dx=0.5-tangent_x,dy=0.5-tangent_y;
    CHECK(dx*dx+dy*dy==radius*radius);
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),tangent_x,tangent_y,true));
    CHECK(bm_flame_hit(BM_FLAME_VIEW(flame),tangent_x+0.001f,tangent_y+0.001f,true));
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),tangent_x-0.001f,tangent_y-0.001f,true));

    const float invalid[]={NAN,INFINITY,-INFINITY};
    for(size_t i=0;i<sizeof invalid/sizeof invalid[0];i++){
        CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),invalid[i],1,true));
        CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),1,invalid[i],true));
    }
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),FLT_MAX,FLT_MAX,true));
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),-FLT_MAX,-FLT_MAX,true));
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),1,FLT_MAX,true));
    memset(flame,0,sizeof flame);flame[MAP_H-1][MAP_W-1]=true;
    CHECK(bm_flame_hit(BM_FLAME_VIEW(flame),MAP_W-1,MAP_H-1,true));
    CHECK(!bm_flame_hit(BM_FLAME_VIEW(flame),0,0,true));
    printf("Flame hit: %d checks, %d failed\n",checks,failures);
    return failures?1:0;
}
