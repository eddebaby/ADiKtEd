/*
 * main.c   contains main() and initialization functions
 */

#include "globals.h"
#include "scr_actn.h"
#include "lev_data.h"
#include "scr_help.h"
#include "var_utils.h"
#include "input_kb.h"
#include "draw_map.h"
#include "lev_script.h"

const char config_filename[]="map.ini";

void read_init(struct SCRMODE_DATA *scrmode,struct WORKMODE_DATA *workdata)
{
    message_log(" read_init: started");
#if defined(unix) && !defined(GO32)
    levels_path="."SEPARATOR"levels";
#else
    levels_path="."SEPARATOR"levels";
#endif
    data_path="."SEPARATOR"data";
    char buffer[READ_BUFSIZE];
    char *p;
    int l;
    
    FILE *fp;
    fp = fopen (config_filename, "rb");
    // If we can't get anything, warn but don't die
    if (!fp)
    {
      message_info_force("Couldn't open \"%s\", defaults loaded.",config_filename);
      return;
    } else
    {
      message_info_force("Welcome to %s. Press F1 for help.",PROGRAM_NAME);
    }
    while (fgets(buffer, READ_BUFSIZE-1, fp))
    {
      //Searchinf for a line containing equal sign
      p = strchr (buffer, '=');
      if ((p==NULL)||(buffer[0]==';'))
          continue;
      strip_crlf(buffer);
      (*p)=0;
      p++;
      while ((*p==' ')||(*p=='\t')) p++;
      int spacepos=strlen(buffer)-1;
      while (spacepos>0)
      {
          if ((buffer[spacepos]==' ')||(buffer[spacepos]=='\t')||(buffer[spacepos]=='\n'))
          {
            buffer[spacepos]='\0';
            spacepos--;
          } else
          {
            spacepos=-1;
          }
      }
      spacepos=strlen(p)-1;
      while (spacepos>0)
      {
          if ((p[spacepos]==' ')||(p[spacepos]=='\t')||(p[spacepos]=='\n'))
          {
            p[spacepos]='\0';
            spacepos--;
          } else
          {
            spacepos=-1;
          }
      }
      if (!strcmp(buffer, "SHOW_OBJ_RANGE"))
      {
          workdata->mapmode->show_obj_range=atoi(p);
          message_log(" read_init: show_obj_range set to %d",(int)workdata->mapmode->show_obj_range);
      } else
      if (!strcmp(buffer, "DAT_VIEW_MODE"))
      {
          workdata->ipanel->dat_view_mode=atoi(p);
          message_log(" read_init: dat_view_mode set to %d",(int)workdata->ipanel->dat_view_mode);
      } else
      if (!strcmp(buffer, "BITMAP_SCALE"))
      {
          bitmap_rescale=atoi(p);
          message_log(" read_init: bitmap_rescale set to %d",(int)bitmap_rescale);
      } else
      if (!strcmp(buffer, "LEVEL_PREVIEW"))
      {
          workdata->mapmode->level_preview=atoi(p);
          message_log(" read_init: level_preview set to %d",(int)workdata->mapmode->level_preview);
      } else
      if (!strcmp(buffer, "FILL_REINFORCED_CORNER"))
      {
          workdata->optns->fill_reinforced_corner=atoi(p);
          message_log(" read_init: fill_reinforced_corner set to %d",(int)workdata->optns->fill_reinforced_corner);
      } else
      if (!strcmp(buffer, "UNAFFECTED_GEMS"))
      {
          workdata->optns->unaffected_gems=atoi(p);
          message_log(" read_init: unaffected_gems set to %d",(int)workdata->optns->unaffected_gems);
      } else
      if (!strcmp(buffer, "UNAFFECTED_ROCK"))
      {
          workdata->optns->unaffected_rock=atoi(p);
          message_log(" read_init: unaffected_rock set to %d",(int)workdata->optns->unaffected_rock);
      } else
      if (!strcmp(buffer, "FRAIL_COLUMNS"))
      {
          workdata->optns->frail_columns=atoi(p);
          message_log(" read_init: frail_columns set to %d",(int)workdata->optns->frail_columns);
      } else

      if (!strcmp(buffer, "TRAPS_LIST_ON_CREATE"))
      {
          workdata->mapmode->traps_list_on_create=atoi(p);
          message_log(" read_init: traps_list_on_create set to %d",(int)workdata->mapmode->traps_list_on_create);
      } else
      if (!strcmp(buffer, "ROOMEFFECT_LIST_ON_CREATE"))
      {
          workdata->mapmode->roomeffect_list_on_create=atoi(p);
          message_log(" read_init: roomeffect_list_on_create set to %d",(int)workdata->mapmode->roomeffect_list_on_create);
      } else
      if (!strcmp(buffer, "ITEMS_LIST_ON_CREATE"))
      {
          workdata->mapmode->items_list_on_create=atoi(p);
          message_log(" read_init: items_list_on_create set to %d",(int)workdata->mapmode->items_list_on_create);
      } else
      if (!strcmp(buffer, "CREATURE_LIST_ON_CREATE"))
      {
          workdata->mapmode->creature_list_on_create=atoi(p);
          message_log(" read_init: creature_list_on_create set to %d",(int)workdata->mapmode->creature_list_on_create);
      } else
      if (!strcmp(buffer, "SCRIPT_LEVEL_SPACES"))
      {
          script_level_spaces=atoi(p);
          message_log(" read_init: script_level_spaces set to %d",(int)script_level_spaces);
      } else
      if (!strcmp(buffer, "DISPLAY_FLOAT_POS"))
      {
          workdata->ipanel->display_float_pos=atoi(p);
          message_log(" read_init: display_float_pos set to %d",(int)workdata->ipanel->display_float_pos);
      } else
      if (!strcmp(buffer, "LEVELS_PATH"))
      {
          levels_path=strdup(p);
          l = strlen(levels_path);
          if (l)
            if (levels_path[l-1]==SEPARATOR[0])
                levels_path[l-1]=0;
          message_log(" read_init: levels_path set to \"%s\"",levels_path);
      } else
      if (!strcmp(buffer, "DATA_PATH"))
      {
          data_path=strdup(p);
          l = strlen(data_path);
          if (l)
            if (data_path[l-1]==SEPARATOR[0])
                data_path[l-1]=0;
          message_log(" read_init: data_path set to \"%s\"",data_path);
      } else
      {
          message_info_force("Bad command \"%s\" in file \"%s\".",buffer,config_filename);
      }
    }
    message_log(" read_init: finished");
}

void get_command_line_options(struct SCRMODE_DATA *scrmode,struct WORKMODE_DATA *workdata,int argc, char **argv)
{
// Note: Automated commands are now just keys inserted into keyboard input buffer.
// in the future, this should be a script generated using input parameters
    int cmnds_count=0;
    short get_msgout_fname=false;
    short get_savout_fname=false;
    int i;
    for (i=1;i<argc;i++)
    {
      char *comnd=argv[i];
      if (strlen(comnd)<1) continue;
      if ((comnd[0]=='-')||(comnd[0]=='/'))
      { //Setting program option
        get_msgout_fname=false;
        get_savout_fname=false;
        if (strcmp(comnd+1,"v")==0)
        {
          scrmode->automated_commands[cmnds_count]='v';
          cmnds_count++;
        } else
        if ((strcmp(comnd+1,"q")==0)||(strcmp(comnd+1,"Q")==0))
        {
          scrmode->automated_commands[cmnds_count]=KEY_CTRL_Q;
          cmnds_count++;
        } else
        if (strcmp(comnd+1,"r")==0)
        {
          scrmode->automated_commands[cmnds_count]=KEY_CTRL_R;
          cmnds_count++;
        } else
        if (strcmp(comnd+1,"n")==0)
        {
          scrmode->automated_commands[cmnds_count]=KEY_CTRL_N;
          cmnds_count++;
        } else
        if (strcmp(comnd+1,"bmp")==0)
        {
          scrmode->automated_commands[cmnds_count]=KEY_CTRL_B;
          cmnds_count++;
        } else
        if (strcmp(comnd+1,"ds")==0)
        {
          disable_sounds=true;
        } else
        if (strcmp(comnd+1,"du")==0)
        {
          datclm_auto_update=false;
          obj_auto_update=false;
        } else
        if (strcmp(comnd+1,"dvid")==0)
        {
          scrmode->screen_enabled=false;
        } else
        if (strcmp(comnd+1,"dinp")==0)
        {
          scrmode->input_enabled=false;
        } else
        if (strcmp(comnd+1,"s")==0)
        {
          scrmode->automated_commands[cmnds_count]=KEY_F5;
          cmnds_count++;
          get_savout_fname=true;
        } else
        if (strcmp(comnd+1,"m")==0)
        {
          get_msgout_fname=true;
        } else
        message_info_force("Unrecognized command line option: \"%s\".",comnd);
      } else
      { //Setting map name
        if (get_msgout_fname)
        {
          set_msglog_fname(comnd);
        } else
        if (get_savout_fname)
        {
          format_map_fname(workdata->lvl->savfname,comnd);
        } else
        {
          format_map_fname(workdata->lvl->fname,comnd);
        }
      }
    }
}


/*
 * Main function; initializes variables, loads map from commandline
 * parameters and enters the main program loop.
 */
int main(int argc, char **argv)
{
    //Allocate and set basic configuration variables
    struct SCRMODE_DATA *scrmode;
    struct WORKMODE_DATA workdata;
    init_levscr_basics(&scrmode,&workdata);
    // Initialize the message displaying and storing
    init_messages();
    // create object for storing map
    level_init(&(workdata.lvl));
    // read configuration file
    read_init(scrmode,&workdata);
    // Interpreting command line parameters
    get_command_line_options(scrmode,&workdata,argc,argv);
    level_set_options(workdata.lvl,workdata.optns);
    // initing keyboard input and screen output, also all modes.
    init_levscr_modes(scrmode,&workdata);
    // Load a map, or start new one
    if (strlen(workdata.lvl->fname)>0)
    {
      popup_show("Loading map","Reading map files. Please wait...");
      load_map(workdata.lvl);
    } else
    {
      start_new_map(workdata.lvl);
    }
    message_log(" main: entering application loop");
    do 
    {
      draw_levscr(scrmode,&workdata);
      proc_key(scrmode,&workdata);
    } while (!finished);
    message_log(" main: application loop finished");
    done(&scrmode,&workdata);
    return 0;
}
