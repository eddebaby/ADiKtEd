/*
 * scr_list.h header file for scr_list.c
 */

#ifndef ADIKT_SCRLIST_H
#define ADIKT_SCRLIST_H

struct LEVEL;
struct SCRMODE_DATA;
struct MAPMODE_DATA;

#define USRINPUT_ROWS 4

//Variables for navigating in list screen

typedef struct {
    int prevmode;
    int y; //the first visible row
    int pos; // selected item position (rel. to screen top)
    int items; //number of items in the list
    int cols; //number of columns in the item list
    // Parameters for specific lists
    int val1;
    int val2;
    int val3;
  } LIST_DATA;

extern LIST_DATA *list;

//Functions - init and free
short init_list(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode);
void free_list(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode);

//Functions - start and stop
short start_mdtextr(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_mdtextr(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_mdslbl(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_mdslbl(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_crtre(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_crtre(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_itemt(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_itemt(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_mdcclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_mdcclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_mdsrch(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_mdsrch(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);

short start_mdlmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_mdlmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_mdsmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void end_mdsmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);

//Functions - start and stop, lower level
short start_list(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int lstmode);
void end_list(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
short start_rplist(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int lstmode);
void end_rplist(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);

//Functions - actions and screen
void draw_crtre(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_crtre(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_itemt(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_itemt(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_mdtextr(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdtextr(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_mdcclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdcclm(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_mdcube(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdcube(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_mdslbl(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdslbl(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_mdsrch(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdsrch(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);

void draw_mdlmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdlmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_mdsmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl);
void actions_mdsmap(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);

//Functions - actions and screen, lower level
short actions_list(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
short actions_rplist(struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,struct LEVEL *lvl,int key);
void draw_numbered_list(char *(*itemstr)(unsigned short),
        struct SCRMODE_DATA *scrmode,struct MAPMODE_DATA *mapmode,
        unsigned int start_idx,unsigned int end_idx,unsigned int itm_width);

//Functions - internal

#endif // ADIKT_SCRLIST_H
