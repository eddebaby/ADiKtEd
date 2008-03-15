/*
 * scr_clm.c
 *
 * Defines functions for initializing and displaying the column screen.
 * This also includes keyboard actions for the screen.
 *
 */

#include "scr_clm.h"

#include "globals.h"
#include "output_scr.h"
#include "input_kb.h"
#include "scr_actn.h"
#include "obj_slabs.h"
#include "obj_column.h"
#include "lev_data.h"

/*
 * Initializes variables for the column screen.
 */
short init_mdclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode)
{
    return true;
}

/*
 * Deallocates memory for the column screen.
 */
void free_mdclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode)
{
}


/*
 * Covers actions from the column screen.
 */
void actions_mdclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key)
{
    int sx, sy;
    sx = (mapmode->map.x+mapmode->screen.x)*3+mapmode->subtl.x;
    sy = (mapmode->map.y+mapmode->screen.y)*3+mapmode->subtl.y;
    message_release();
    
    if (!cursor_actions(scrmode,mapmode,lvl,key))
    if (!subtl_select_actions(mapmode,key))
    {
      switch (key)
      {
        case KEY_TAB:
        case KEY_ESCAPE:
          end_mdclm(scrmode,mapmode,lvl);
          break;
        case KEY_U: // Update all things/dat/clm/w?b
          update_slab_owners(lvl);
          update_datclm_for_whole_map(lvl);
          message_info("DAT/CLM/W?B entries updated.");
          break;
        case KEY_A: // update dat/clm/w?b of selected tile
          {
            //Deleting custom columns - to enable auto-update
            cust_cols_del_for_tile(lvl,sx/3,sy/3);
            //Updating
            update_datclm_for_slab(lvl, sx/3,sy/3);
            update_tile_wib_entries(lvl,sx/3,sy/3);
            update_tile_wlb_entry(lvl,sx/3,sy/3);
            update_tile_flg_entries(lvl,sx/3,sy/3);
            message_info("Updated DAT/CLM/W?B entries of slab %d,%d.",sx/3,sy/3);
          };break;
        case KEY_M: // manual-set mode
          mdstart[MD_CCLM](scrmode,mapmode,lvl);
          break;
        case KEY_B: // cube mode
          mdstart[MD_CUBE](scrmode,mapmode,lvl);
          break;
        default:
          message_info("Unrecognized clm key code: %d",key);
          speaker_beep();
      }
    }
}

/*
 * Action function - start the column mode.
 */
short start_mdclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl)
{
    scrmode->mode=MD_CLM;
    scrmode->usrinput_type=SI_NONE;
    return true;
}

/*
 * Action function - end the column mode.
 */
void end_mdclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl)
{
    mapmode->panel_mode=PV_MODE;
    scrmode->usrinput_type=SI_NONE;
    scrmode->usrinput_pos=0;
    scrmode->usrinput[0]='\0';
    scrmode->mode=MD_SLB;
    message_info("Returned to Slab mode.");
}

int display_dat_subtiles(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int scr_row, int scr_col,int ty,int tx)
{
    int i, k;
    int color;
    // Display .dat stuff for this tile
    screen_setcolor(PRINT_COLOR_LGREY_ON_BLACK);
    set_cursor_pos(scr_row, scr_col);
    scr_row+=2;
    screen_printf("%s",".dat entries");
    for (k=0; k<3; k++)
    {
        int sy=ty*3+k;
        for (i=0; i<3; i++)
        {
            int sx=tx*3+i;
            set_cursor_pos(scr_row, scr_col+i*5);
            
            if ((i==mapmode->subtl.x) && (k==mapmode->subtl.y) &&
             ((scrmode->mode==MD_CLM)||(scrmode->mode==MD_RWRK)) )
                color=PRINT_COLOR_BLACK_ON_LGREY;
            else
                color=PRINT_COLOR_LGREY_ON_BLACK;
            screen_setcolor(color);
            if (mapmode->dat_view_mode==2)
                screen_printf("%4u",
                        (unsigned int)get_dat_subtile(lvl,sx,sy));
            else
                screen_printf("%04X",(unsigned int)get_dat_val(lvl,sx,sy));
        }
        scr_row+=2;
    }
    return scr_row;
}

/*
 * Draws screen for the column mode.
 */
void draw_mdclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl)
{
    draw_map_area(scrmode,mapmode,lvl,true,true,false);
    if (mapmode->panel_mode!=PV_MODE)
      draw_forced_panel(scrmode,mapmode,lvl,mapmode->panel_mode);
    else
      draw_mdclm_panel(scrmode,mapmode,lvl);
    draw_map_cursor(scrmode,mapmode,lvl,true,true,false);
}

void draw_mdclm_panel(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl)
{
    int tx, ty;
    tx = mapmode->screen.x+mapmode->map.x;
    ty = mapmode->screen.y+mapmode->map.y;
    int sx, sy;
    sx = tx*3+mapmode->subtl.x;
    sy = ty*3+mapmode->subtl.y;
    int clm_idx=get_dat_subtile(lvl,sx,sy);
    char *clmentry;
    clmentry = lvl->clm[clm_idx%COLUMN_ENTRIES];
    int pos_y;
    pos_y=display_column(clmentry, clm_idx,0,scrmode->cols+3);
    set_cursor_pos(pos_y-3, scrmode->cols+23);
    screen_printf_toeol("WIB anim.: %03X", get_subtl_wib(lvl,sx,sy));
    set_cursor_pos(pos_y-2, scrmode->cols+23);
    screen_printf("Tile WLB:  %03X", get_tile_wlb(lvl, tx, ty));
    set_cursor_pos(pos_y-1, scrmode->cols+23);
    screen_printf("Subtl.FLG: %03X", get_subtl_flg(lvl,sx,sy));
    display_rpanel_bottom(scrmode,mapmode,lvl);
}

int display_column(unsigned char *clmentry,int clm_idx, int scr_row, int scr_col)
{
    struct COLUMN_REC *clm_rec;
    clm_rec=create_column_rec();
    get_clm_entry(clm_rec, clmentry);
    screen_setcolor(PRINT_COLOR_LGREY_ON_BLACK);
    int scr_col2=scr_col+20;
    set_cursor_pos(scr_row++, scr_col);
    screen_printf("Use: %04X (dec: %d)", clm_rec->use,clm_rec->use);
    set_cursor_pos(scr_row, scr_col);
    screen_printf("Lintel: %d", clm_rec->lintel);
    set_cursor_pos(scr_row++, scr_col2);
    screen_printf("CLM index: %4d", clm_idx);
    set_cursor_pos(scr_row, scr_col);
    screen_printf("Solid mask: %04X", clm_rec->solid);
    set_cursor_pos(scr_row++, scr_col2);
    screen_printf("Height: %X", clm_rec->height);
    set_cursor_pos(scr_row, scr_col2);
    screen_printf("Permanent: %d", clm_rec->permanent);
    set_cursor_pos(scr_row+1, scr_col2);
    screen_printf("Orientation: %02X", clm_rec->orientation);
    int i;
    for (i=7; i>=0; i--)
    {
      set_cursor_pos(scr_row++, scr_col);
      screen_printf("Cube %d:  %03X", i, clm_rec->c[i]);
    }
    set_cursor_pos(scr_row++, scr_col);
    screen_printf("Base bl: %03X", clm_rec->base);
    free_column_rec(clm_rec);
    return scr_row;
}
