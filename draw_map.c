/******************************************************************************/
// draw_map.c - Graphics draw of Dungeon Keeper map.
/******************************************************************************/
// Author:   Tomasz Lis
// Created:  12 Jan 2008

// Purpose:
//   Functions needed to create a graphic view of DK1 map using original
//   textures.

// Comment:
//   None.

//Copying and copyrights:
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
/******************************************************************************/

#include "draw_map.h"

#include <math.h>
#include "globals.h"
#include "var_utils.h"
#include "memfile.h"
#include "obj_cube.h"
#include "obj_slabs.h"
#include "lev_data.h"
#include "lev_files.h"
#include "lev_column.h"
#include "obj_column.h"
#include "obj_things.h"
#include "xtabdat8.h"

/*
 * This array contains color intensity added to bitmap where owners should be visible
 */
struct PALETTE_ENTRY owned_area_palette_intns[] =
 { {63,24,11,0}, {47,31,63,0}, {54,63,26,0}, {63,59,0,0}, {52,47,42,0}, {0,0,0,0}, };//intensified
struct PALETTE_ENTRY owned_area_palette_std[] =
 { {31,12,5,0}, {20,13,27,0}, {23,27,11,0}, {31,29,0,0}, {22,19,17,0}, {0,0,0,0}, };//divided by 2
struct PALETTE_ENTRY owned_area_palette_weak[] =
 { {19,7,2,0}, {10,6,15,0}, {11,14,5,0}, {15,14,0,0}, {11,9,8,0}, {0,0,0,0}, };//divided by 4

struct PALETTE_ENTRY intense_slab_palette_std[] =
 { {31,10,17,0}, {28,28,2,0}, };//divided by 2
struct PALETTE_ENTRY intense_slab_palette_weak[] =
 { {15,5,8,0}, {14,14,1,0}, };//divided by 4

short bitmap_rescale=4;

short load_palette(struct PALETTE_ENTRY *pal,char *fname)
{
    //Reading file
    struct MEMORY_FILE mem;
    mem = read_file(fname);
    if (mem.errcode!=MFILE_OK) return mem.errcode;
    if ((mem.len!=256*3))
    {
      free(mem.content);
      return ERR_FILE_BADDATA;
    }
    //Loading the entries
    int i;
    for (i=0; i<256; i++)
    {
        unsigned int item_pos=i*3;
        pal[i].b=mem.content[item_pos+2];
        pal[i].g=mem.content[item_pos+1];
        pal[i].r=mem.content[item_pos+0];
        pal[i].o=0;
    }
    free(mem.content);
    return ERR_NONE;
}

short load_cubedata(struct CUBES_DATA *cubes,char *fname)
{
    //Reading file
    struct MEMORY_FILE mem;
    mem = read_file(fname);
    if (mem.errcode!=MFILE_OK) return mem.errcode;
    if (mem.len<22)
    {
      free(mem.content);
      return ERR_FILE_BADDATA;
    }
    cubes->count=read_short_le_buf(mem.content+0);
    if ((mem.len!=cubes->count*18+4))
    {
      free(mem.content);
      return ERR_FILE_BADDATA;
    }
    //Loading the entries
    cubes->data=malloc(cubes->count*sizeof(struct CUBE_TEXTURES));
    int i;
    for (i=0; i<cubes->count; i++)
    {
        unsigned int item_pos=i*18+4;
        unsigned int val;
        val=read_short_le_buf(mem.content+item_pos+0);
        cubes->data[i].n=val;
        val=read_short_le_buf(mem.content+item_pos+2);
        cubes->data[i].s=val;
        val=read_short_le_buf(mem.content+item_pos+4);
        cubes->data[i].w=val;
        val=read_short_le_buf(mem.content+item_pos+6);
        cubes->data[i].e=val;
        val=read_short_le_buf(mem.content+item_pos+ 8);
        cubes->data[i].t=val;
        val=read_short_le_buf(mem.content+item_pos+10);
        cubes->data[i].b=val;
    }
    free(mem.content);
    return ERR_NONE;
}

short load_textureanim(struct CUBES_DATA *cubes,char *fname)
{
    //Reading file
    struct MEMORY_FILE mem;
    mem = read_file(fname);
    if (mem.errcode!=MFILE_OK) return mem.errcode;
    cubes->anitxcount=(mem.len>>4);
    if ((mem.len!=(cubes->anitxcount<<4)))
    {
      free(mem.content);
      return ERR_FILE_BADDATA;
    }
    //Loading the entries
    cubes->anitx=malloc(cubes->anitxcount*sizeof(struct CUBE_TXTRANIM));
    int i,k;
    for (i=0; i<cubes->anitxcount; i++)
      for (k=0; k<8; k++)
      {
        unsigned int item_pos=i*16 + k*2;
        unsigned int val;
        val=read_short_le_buf(mem.content+item_pos);
        cubes->anitx[i].data[k]=val;
      }
    free(mem.content);
    return ERR_NONE;
}

short load_texture(struct MAPDRAW_DATA *draw_data,char *fname)
{
    unsigned long texture_file_len = (TEXTURE_SIZE_X*TEXTURE_COUNT_X) * (TEXTURE_SIZE_Y*TEXTURE_COUNT_Y);
    //Reading file
    struct MEMORY_FILE mem;
    mem = read_file(fname);
    if (mem.errcode!=MFILE_OK) return mem.errcode;
    if ((mem.len!=texture_file_len))
    {
      free(mem.content);
      return ERR_FILE_BADDATA;
    }
    // Allocating buffer
    draw_data->texture=malloc(texture_file_len);
    if (draw_data->texture==NULL)
      die("load_texture: Out of memory.");
    //Loading the entries
    int i;
    for (i=0; i<(TEXTURE_SIZE_Y*TEXTURE_COUNT_Y); i++)
    {
        unsigned long pos = (TEXTURE_SIZE_X*TEXTURE_COUNT_X) * i;
        memcpy(draw_data->texture+pos,mem.content+pos,(TEXTURE_SIZE_X*TEXTURE_COUNT_X));
    }
    free(mem.content);
    return ERR_NONE;
}

short place_sprite_on_buf_rgb(unsigned char *dest,const struct IPOINT_2D dest_pos,
    const struct IPOINT_2D dest_size,struct PALETTE_ENTRY *pal,const struct IMAGEITEM *spr)
{
    long dest_idx=((dest_pos.y-(spr->height>>1))*dest_size.x*3);
    unsigned long src_idx=0;
    unsigned long dest_fullsize=dest_size.x*dest_size.y*3;
    int w,h;
    for (h=0;h<spr->height;h++)
    {
      if (dest_idx<0) continue;
      if (dest_idx>=dest_fullsize) break;
      for (w=0;w<spr->width;w++)
      {
          long dest_sidx=dest_pos.x-(spr->width>>1)+w;
          if ((dest_sidx<0)||(dest_sidx>=dest_size.x)) continue;
          //Using multiplication to place colour on the pixel
          struct PALETTE_ENTRY *pxdata;
          pxdata=&pal[spr->data[src_idx+w]];
          unsigned short alpha=spr->alpha[src_idx+w];
          unsigned short nvalr,nvalg,nvalb;
          nvalb=(dest[dest_idx+(dest_sidx*3)+0]);
          nvalg=(dest[dest_idx+(dest_sidx*3)+1]);
          nvalr=(dest[dest_idx+(dest_sidx*3)+2]);
          dest[dest_idx+(dest_sidx*3)+0]=(alpha*nvalb+(255-alpha)*(pxdata->b<<2))>>8;
          dest[dest_idx+(dest_sidx*3)+1]=(alpha*nvalg+(255-alpha)*(pxdata->g<<2))>>8;
          dest[dest_idx+(dest_sidx*3)+2]=(alpha*nvalr+(255-alpha)*(pxdata->r<<2))>>8;
      }
      dest_idx+=(3*dest_size.x);
      src_idx+=spr->width;
    }
    return true;
}

short draw_rect_mul_on_buffer(unsigned char *dest,const struct IPOINT_2D dest_pos,
    const struct IPOINT_2D dest_size,const struct IPOINT_2D rect_size,
    const struct PALETTE_ENTRY *pxdata,const struct IPOINT_2D scale)
{
    unsigned long dest_idx=(dest_pos.y*dest_size.x*3);
    unsigned long dest_fullsize=dest_size.x*dest_size.y*3;
    int i,j;
    for (j=0;j<rect_size.y;j++)
    {
      if (dest_idx>=dest_fullsize) break;
      if ((j%scale.y)>0)
          continue;
      for (i=0;i<rect_size.x;i++)
      {
          if ((i%scale.x)!=0) continue;
          unsigned long dest_sidx=dest_pos.x+(i/scale.x);
          if (dest_sidx>=dest_size.x) continue;
          //Using multiplication to place colour on the pixel
          unsigned char nvalr,nvalg,nvalb;
          nvalb=(dest[dest_idx+(dest_sidx*3)+0]);
          nvalg=(dest[dest_idx+(dest_sidx*3)+1]);
          nvalr=(dest[dest_idx+(dest_sidx*3)+2]);
          unsigned short sum=nvalr+nvalg+nvalb;
          dest[dest_idx+(dest_sidx*3)+0]=min(nvalb+((sum*pxdata->b)>>7),255);
          dest[dest_idx+(dest_sidx*3)+1]=min(nvalg+((sum*pxdata->g)>>7),255);
          dest[dest_idx+(dest_sidx*3)+2]=min(nvalr+((sum*pxdata->r)>>7),255);
      }
      dest_idx+=(3*dest_size.x);
    }
    return true;
}

short draw_rect_sum_on_buffer(unsigned char *dest,const struct IPOINT_2D dest_pos,
    const struct IPOINT_2D dest_size,const struct IPOINT_2D rect_size,
    const struct PALETTE_ENTRY *pxdata,const struct IPOINT_2D scale)
{
    unsigned long dest_idx=(dest_pos.y*dest_size.x*3);
    unsigned long dest_fullsize=dest_size.x*dest_size.y*3;
    int i,j;
    for (j=0;j<rect_size.y;j++)
    {
      if (dest_idx>=dest_fullsize) break;
      if ((j%scale.y)>0)
          continue;
      for (i=0;i<rect_size.x;i++)
      {
          if ((i%scale.x)!=0) continue;
          unsigned long dest_sidx=dest_pos.x+(i/scale.x);
          if (dest_sidx>=dest_size.x) continue;
          //Using sum place colour on the pixel
          unsigned char nval;
          nval=(dest[dest_idx+(dest_sidx*3)+0]);
          dest[dest_idx+(dest_sidx*3)+0]=min(nval+pxdata->b,255);
          nval=(dest[dest_idx+(dest_sidx*3)+1]);
          dest[dest_idx+(dest_sidx*3)+1]=min(nval+pxdata->g,255);
          nval=(dest[dest_idx+(dest_sidx*3)+2]);
          dest[dest_idx+(dest_sidx*3)+2]=min(nval+pxdata->r,255);
      }
      dest_idx+=(3*dest_size.x);
    }
    return true;
}

short draw_texture_on_buffer(unsigned char *dest,const struct IPOINT_2D dest_pos,
    const struct IPOINT_2D dest_size,const unsigned char *src,const struct IPOINT_2D src_pos,
    const struct IPOINT_2D src_size,const struct IPOINT_2D rect_size,
    struct PALETTE_ENTRY *pal,const struct IPOINT_2D scale)
{
    unsigned long dest_idx=(dest_pos.y*dest_size.x*3);
    unsigned long src_idx=(src_pos.y*src_size.x);
    unsigned long src_fullsize=src_size.x*src_size.y;
    unsigned long dest_fullsize=dest_size.x*dest_size.y*3;
    int i,j;
    for (j=0;j<rect_size.y;j++)
    {
      if (dest_idx>=dest_fullsize) break;
      if (src_idx>=src_fullsize) break;
      if ((j%scale.y)>0)
      {
          src_idx+=src_size.x;
          continue;
      }
      int src_add=rnd(scale.y)*src_size.x;
      if (src_idx+src_add>=src_fullsize) break;
      unsigned short ridy=rnd(256);
      if (scale.x>7)
        for (i=0;i<rect_size.x;i++)
        {
          if ((i%scale.x)!=(ridy%scale.x)) continue;
          unsigned long dest_sidx=dest_pos.x+(i/scale.x);
          if (dest_sidx>=dest_size.x) continue;
          if ((src_pos.x+i)>=src_size.x) continue;
          unsigned short ridx=rnd(1);
          struct PALETTE_ENTRY *pxdata1;
          if ((src_pos.x+i+ridx)>=src_size.x)
            pxdata1=&pal[src[src_idx+src_add+src_pos.x+i]];
          else
            pxdata1=&pal[src[src_idx+src_add+src_pos.x+i+ridx]];
          struct PALETTE_ENTRY *pxdata2;
          if ((src_pos.x+i+ridx+2)>=src_size.x)
            pxdata2=pxdata1;
          else
            pxdata2=&pal[src[src_idx+src_add+src_pos.x+i+ridx+2]];
          struct PALETTE_ENTRY *pxdata3;
          if ((src_pos.x+i+ridx+4)>=src_size.x)
            pxdata3=pxdata1;
          else
            pxdata3=&pal[src[src_idx+src_add+src_pos.x+i+ridx+4]];
          struct PALETTE_ENTRY *pxdata4;
          if ((src_pos.x+i+ridx+6)>=src_size.x)
            pxdata4=pxdata1;
          else
            pxdata4=&pal[src[src_idx+src_add+src_pos.x+i+ridx+6]];
          dest[dest_idx+(dest_sidx*3)+0]=(pxdata1->b)+(pxdata2->b)+(pxdata3->b)+(pxdata4->b);
          dest[dest_idx+(dest_sidx*3)+1]=(pxdata1->g)+(pxdata2->g)+(pxdata3->g)+(pxdata4->g);
          dest[dest_idx+(dest_sidx*3)+2]=(pxdata1->r)+(pxdata2->r)+(pxdata3->r)+(pxdata4->r);
        }
      else
      if (scale.x>3)
        for (i=0;i<rect_size.x;i++)
        {
          if ((i%scale.x)!=(ridy%scale.x)) continue;
          unsigned long dest_sidx=dest_pos.x+(i/scale.x);
          if (dest_sidx>=dest_size.x) continue;
          if ((src_pos.x+i)>=src_size.x) continue;
          unsigned short ridx=rnd(1);
          struct PALETTE_ENTRY *pxdata1;
          if ((src_pos.x+i+ridx)>=src_size.x)
            pxdata1=&pal[src[src_idx+src_add+src_pos.x+i]];
          else
            pxdata1=&pal[src[src_idx+src_add+src_pos.x+i+ridx]];
          struct PALETTE_ENTRY *pxdata2;
          if ((src_pos.x+i+ridx+2)>=src_size.x)
            pxdata2=pxdata1;
          else
            pxdata2=&pal[src[src_idx+src_add+src_pos.x+i+ridx+2]];
          dest[dest_idx+(dest_sidx*3)+0]=(pxdata1->b<<1)+(pxdata2->b<<1);
          dest[dest_idx+(dest_sidx*3)+1]=(pxdata1->g<<1)+(pxdata2->g<<1);
          dest[dest_idx+(dest_sidx*3)+2]=(pxdata1->r<<1)+(pxdata2->r<<1);
        }
      else
        for (i=0;i<rect_size.x;i++)
        {
          if ((i%scale.x)!=(ridy%scale.x)) continue;
          unsigned long dest_sidx=dest_pos.x+(i/scale.x);
          if (dest_sidx>=dest_size.x) continue;
          if ((src_pos.x+i)>=src_size.x) continue;
          struct PALETTE_ENTRY *pxdata=&pal[src[src_idx+src_add+src_pos.x+i]];
          dest[dest_idx+(dest_sidx*3)+0]=(pxdata->b<<2);
          dest[dest_idx+(dest_sidx*3)+1]=(pxdata->g<<2);
          dest[dest_idx+(dest_sidx*3)+2]=(pxdata->r<<2);
        }
      dest_idx+=(3*dest_size.x);
      src_idx+=src_size.x;
    }
    return true;
}

/*
 * returns texture coords for top of the cube with given index
 */
short get_top_texture_pos(struct IPOINT_2D *texture_pos,const struct CUBES_DATA *cubes,unsigned short cube_idx)
{
    if ((cube_idx>=0x021b)&&(cube_idx<0x025f))
    //Water and lava hack, to skip loading of cube groups file
    {
        texture_pos->x=(rnd(8))*TEXTURE_SIZE_X;
        texture_pos->y=(cube_idx-0x021b)*TEXTURE_SIZE_Y;
    } else
    {
        // Retrieving texture top index
        unsigned int cube_top;
        if (cube_idx<cubes->count)
        {
            cube_top=cubes->data[cube_idx].t;
        } else
        {
            cube_top=0;
        }
        // Checking if the index is animated
        if (cube_top>=TEXTURE_COUNT_X*TEXTURE_COUNT_Y)
        {
            cube_top-=TEXTURE_COUNT_X*TEXTURE_COUNT_Y;
            if (cube_top<cubes->anitxcount)
            {
              cube_top=cubes->anitx[cube_top].data[rnd(8)];
            } else
            {
              cube_top=0;
            }
        }
        texture_pos->x=(cube_top&7)*TEXTURE_SIZE_X;
        texture_pos->y=((cube_top>>3)&127)*TEXTURE_SIZE_Y;
    }
    return true;
}

short draw_map_on_buffer(char *dst,const struct LEVEL *lvl,const struct MAPDRAW_DATA *draw_data)
{
    struct IPOINT_2D texture_size={TEXTURE_SIZE_X*TEXTURE_COUNT_X,TEXTURE_SIZE_Y*TEXTURE_COUNT_Y};
    struct IPOINT_2D single_txtr_size={TEXTURE_SIZE_X,TEXTURE_SIZE_Y};
    struct IPOINT_2D dest_size={draw_data->end.x-draw_data->start.x+1,draw_data->end.y-draw_data->start.y+1};
    struct IPOINT_2D texture_pos;
    struct IPOINT_2D dest_pos;
    struct IPOINT_2D scale={1<<(draw_data->rescale),1<<(draw_data->rescale)};
    struct IPOINT_2D scaled_txtr_size={TEXTURE_SIZE_X>>(draw_data->rescale),TEXTURE_SIZE_Y>>(draw_data->rescale)};
    //Finding start/end of the drawing area
    struct IPOINT_2D start;
    start.x = draw_data->start.x/scaled_txtr_size.x;
    start.y = draw_data->start.y/scaled_txtr_size.y;
    struct IPOINT_2D end;
    end.x = draw_data->end.x/scaled_txtr_size.x + ((draw_data->end.x%scaled_txtr_size.x)>0);
    end.y = draw_data->end.y/scaled_txtr_size.y + ((draw_data->end.y%scaled_txtr_size.y)>0);
    int i,j;
    for (j=start.y; j<end.y; j++)
    {
      dest_pos.y=j*(scaled_txtr_size.y);
      for (i=start.x; i<end.x; i++)
      {
          unsigned short cube_idx;
          unsigned char *clmentry;
          clmentry=get_subtile_column(lvl,i,j);
          cube_idx=get_clm_entry_topcube(clmentry);
          get_top_texture_pos(&texture_pos,&(draw_data->cubes),cube_idx);
          dest_pos.x=i*(scaled_txtr_size.x);
          draw_texture_on_buffer(dst,dest_pos,dest_size,draw_data->texture,
              texture_pos,texture_size,single_txtr_size,draw_data->palette,scale);
      }
    }
    if (draw_data->ownerpal!=NULL)
      for (j=start.y; j<end.y; j++)
      {
        dest_pos.y=j*(scaled_txtr_size.y);
        for (i=start.x; i<end.x; i++)
        {
          dest_pos.x=i*(scaled_txtr_size.x);
          unsigned short slab=get_tile_slab(lvl,i/MAP_SUBNUM_X,j/MAP_SUBNUM_Y);
          unsigned char owner=get_subtl_owner(lvl,i,j);
          if (slab==SLAB_TYPE_GOLD)
          {
            if (draw_data->intnspal!=NULL)
              draw_rect_mul_on_buffer(dst,dest_pos,dest_size,single_txtr_size,
                &(draw_data->intnspal[1]),scale);
          } else
          if (slab==SLAB_TYPE_GEMS)
          {
            if (draw_data->intnspal!=NULL)
              draw_rect_mul_on_buffer(dst,dest_pos,dest_size,single_txtr_size,
                &(draw_data->intnspal[0]),scale);
          } else
          {
            draw_rect_mul_on_buffer(dst,dest_pos,dest_size,single_txtr_size,
              &(draw_data->ownerpal[owner%6]),scale);
          }
        }
      }
    return true;
}

short draw_things_on_buffer(char *dst,const struct LEVEL *lvl,const struct MAPDRAW_DATA *draw_data)
{
    struct IPOINT_2D scaled_txtr_size={(TEXTURE_SIZE_X>>(draw_data->rescale)),(TEXTURE_SIZE_Y>>(draw_data->rescale))};
    //Finding start/end subtile of the drawing area
    struct IPOINT_2D start;
    start.x = draw_data->start.x/scaled_txtr_size.x;
    start.y = draw_data->start.y/scaled_txtr_size.y;
    struct IPOINT_2D end;
    end.x = draw_data->end.x/scaled_txtr_size.x + ((draw_data->end.x%scaled_txtr_size.x)>0);
    end.y = draw_data->end.y/scaled_txtr_size.y + ((draw_data->end.y%scaled_txtr_size.y)>0);
    struct IPOINT_2D dest_scaled_size={(end.x-start.x)*scaled_txtr_size.x,(end.y-start.y)*scaled_txtr_size.y};
    struct IPOINT_2D dest_pos;
    // Looping through coords and placing things - first pass, the background things
    int i,j,k;
    for (j=start.y; j<end.y; j++)
    {
        for (i=start.x; i<end.x; i++)
        {
          int last_obj=get_thing_subnums(lvl,i,j)-1;
          for (k=last_obj; k>=0; k--)
          {
            unsigned char *thing=get_thing(lvl,i,j,k);
            int spr_idx=-1;
            if (is_gold(thing))
            {
              //Show only some of the gold
              if (rnd(7)==0)
                spr_idx=510;
            } else
            if (is_food(thing))
            {
              spr_idx=313;
            } else
            if (is_spellbook(thing))
            {
              spr_idx=60;
            } else
            if (is_trainpost(thing))
            {
              spr_idx=66;
            }
            dest_pos.x=i*(scaled_txtr_size.x);
            dest_pos.y=j*(scaled_txtr_size.y);
            if ((spr_idx>=0)&&(spr_idx<(draw_data->images->count)))
            {
              struct IMAGEITEM *item=&(draw_data->images->items[spr_idx]);
              place_sprite_on_buf_rgb(dst,dest_pos,dest_scaled_size,draw_data->palette,item);
            }
          }
        }
    }
    //Second pass - foreground things
    for (j=start.y; j<end.y; j++)
    {
        for (i=start.x; i<end.x; i++)
        {
          int last_obj=get_thing_subnums(lvl,i,j)-1;
          for (k=last_obj; k>=0; k--)
          {
            unsigned char *thing=get_thing(lvl,i,j,k);
            int spr_idx=-1;
            if (is_creature(thing))
            {
              unsigned short subtp=get_thing_subtype(thing);
              if (subtp<=CREATR_SUBTP_TENTCL)
                spr_idx=175+(2*subtp);
              else
              if (subtp==CREATR_SUBTP_ORC)
                spr_idx=495;
              else
                spr_idx=497;
            } else
            if (is_dngspecbox(thing))
            {
              spr_idx=163;
            }
            dest_pos.x=i*(scaled_txtr_size.x);
            dest_pos.y=j*(scaled_txtr_size.y);
            if ((spr_idx>=0)&&(spr_idx<(draw_data->images->count)))
            {
              struct IMAGEITEM *item=&(draw_data->images->items[spr_idx]);
              place_sprite_on_buf_rgb(dst,dest_pos,dest_scaled_size,draw_data->palette,item);
            }
          }
        }
    }
    return true;
}

short write_bitmap_rgb(const char *fname,const unsigned char *data,const struct IPOINT_2D size)
{
    if ((fname==NULL)||(data==NULL))
    {
      message_error("Internal error - null poiner detected in write_bitmap_rgb");
      return false;
    }

    long l;
    int i, j;
    FILE *out;
    out = fopen (fname, "wb");
    if (out==NULL)
    {
      message_error("Can't open \"%s\" for writing", fname);
      return false;
    }
    
    l = size.x*size.y*3;
    fprintf (out, "BM");
    write_long_le_file (out, l+0x36);
    write_long_le_file (out, 0);
    write_long_le_file (out, 0x36);
    write_long_le_file (out, 40);
    write_long_le_file (out, size.x);
    write_long_le_file (out, -size.y);
    write_short_le_file (out, 1);
    write_short_le_file (out, 24);
    write_long_le_file (out, 0);
    write_long_le_file (out, 0);
    write_long_le_file (out, 0);
    write_long_le_file (out, 0);
    write_long_le_file (out, 0);
    write_long_le_file (out, 0);
    
    for (i=0; i < size.y; i++)
    {
      fwrite (data+(i*(size.x*3)), size.x*3, 1, out);
	  if ((size.x*3) & 3)
      {
	    for (j=0; j < 4-((size.x*3)&3); j++)
		fputc (0, out);
      }
    }
    fclose (out);
    return true;
}

/*
 * Generates bitmap representing the current map layout
 */
short generate_map_bitmap(const char *bmpfname,const struct LEVEL *lvl,const short rescale)
{
    short result;
    struct MAPDRAW_DATA *draw_data=malloc(sizeof(struct MAPDRAW_DATA));
    if (draw_data==NULL)
      die("generate_map_bitmap: Out of memory.");
    // Initializing draw_data values
    draw_data->cubes.data=NULL;
    draw_data->cubes.count=0;
    draw_data->texture=NULL;
    draw_data->palette=malloc(256*sizeof(struct PALETTE_ENTRY));
    draw_data->images=malloc(sizeof(struct IMAGELIST));
    if (rescale>3)
    {
      draw_data->ownerpal=owned_area_palette_std;
      draw_data->intnspal=intense_slab_palette_weak;
    } else
    {
      draw_data->ownerpal=NULL;
      draw_data->intnspal=NULL;
    }
    if (draw_data->palette==NULL)
      die("generate_map_bitmap: Out of memory.");
    struct IPOINT_2D textr_size={TEXTURE_SIZE_X>>rescale,TEXTURE_SIZE_Y>>rescale};
    struct IPOINT_2D bmp_size;
    // Settings to draw whole map
    bmp_size.x=textr_size.x*MAP_SIZE_X*MAP_SUBNUM_X;
    bmp_size.y=textr_size.y*MAP_SIZE_Y*MAP_SUBNUM_Y;
    draw_data->start.x=0;
    draw_data->start.y=0;
    draw_data->end.x=bmp_size.x-draw_data->start.x-1;
    draw_data->end.y=bmp_size.y-draw_data->start.y-1;
    draw_data->rescale=rescale;
    unsigned char *bitmap=NULL;
    // Loading files needed to draw
    char *fnames;
    result=true;
    if (result)
    {
      fnames=NULL;
      format_data_fname(&fnames,"palette.dat");
      result = (load_palette(draw_data->palette,fnames)==ERR_NONE);
      if (!result)
          message_error("Error when loading file \"%s\"",fnames);
      free(fnames);
    }
    if (result)
    {
      fnames=NULL;
      format_data_fname(&fnames,"cube.dat");
      result = (load_cubedata(&(draw_data->cubes),fnames)==ERR_NONE);
      if (!result)
          message_error("Error when loading file \"%s\"",fnames);
      free(fnames);
    }
    if (result)
    {
      fnames=NULL;
      format_data_fname(&fnames,"tmapanim.dat");
      result = (load_textureanim(&(draw_data->cubes),fnames)==ERR_NONE);
      if (!result)
          message_error("Error when loading file \"%s\"",fnames);
      free(fnames);
    }
    if (result)
    {
      fnames=NULL;
      format_data_fname(&fnames,"tmapa%03d.dat",(int)(lvl->inf%8));
      result = (load_texture(draw_data,fnames)==ERR_NONE);
      if (!result)
          message_error("Error when loading file \"%s\"",fnames);
      free(fnames);
    }
    //Reading DAT,TAB and extracting images
    if (result)
    {
      char *tabfname;
      fnames=NULL;
      tabfname=NULL;
      int large_tngicons=(rescale<3);
      format_data_fname(&fnames,"gui2-0-%d.dat",large_tngicons);
      format_data_fname(&tabfname,"gui2-0-%d.tab",large_tngicons);
      result = (create_images_dattab_idx(draw_data->images,fnames,tabfname,1)==ERR_NONE);
      if (!result)
          message_error("Error when loading dat/tab pair \"%s\"",fnames);
      free(fnames);
      free(tabfname);
    }
    if (result)
    {
      bitmap=(unsigned char *)malloc((bmp_size.x*bmp_size.y+1)*3);
      if (bitmap==NULL)
        die("generate_map_bitmap: Out of memory.");
    }
    if (result)
    {
      result = draw_map_on_buffer(bitmap,lvl,draw_data);
      if (!result)
          message_error("Error when drawing map on memory buffer");
    }
    if (result)
    {
      if (rescale<5)
        result = draw_things_on_buffer(bitmap,lvl,draw_data);
      if (!result)
          message_error("Error when placing thing sprites on memory buffer");
    }
    if (result)
    {
      //result = write_bitmap_idx(bmpfname, draw_data->palette, bitmap, bmp_size);
      result = write_bitmap_rgb(bmpfname,bitmap,bmp_size);
    }
    free(bitmap);
    free_dattab_images(draw_data->images);
    free(draw_data->images);
    free(draw_data->cubes.data);
    free(draw_data->texture);
    free(draw_data->palette);
    free(draw_data);
    return result;
}

/*
 * Generates bitmap representing the current map layout
 * Saves it on the filename same as level name, and with BMP extension
 */
short generate_map_bitmap_mapfname(struct LEVEL *lvl)
{
    short result;
    char *bmpfname;
    bmpfname = (char *)malloc(strlen(lvl->fname)+5);
    if (bmpfname==NULL)
        die("generate_map_bitmap: Out of memory.");
    sprintf (bmpfname, "%s.bmp", lvl->fname);
    result = generate_map_bitmap(bmpfname,lvl,bitmap_rescale);
    if (result)
      message_info("Bitmap \"%s\" created.",bmpfname);
    free(bmpfname);
    return result;
}

