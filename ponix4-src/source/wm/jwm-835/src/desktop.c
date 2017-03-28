/**
 * @file desktop.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the desktop management functions.
 *
 */

#include "jwm.h"
#include "desktop.h"
#include "main.h"
#include "client.h"
#include "clientlist.h"
#include "hint.h"
#include "pager.h"
#include "taskbar.h"
#include "error.h"
#include "menu.h"
#include "misc.h"
#include "background.h"
#include "settings.h"

char **desktopNames = NULL;

char showingDesktop;

/** Startup desktop support. */
void StartupDesktops()
{

   unsigned int x;

   if(desktopNames == NULL) {
      desktopNames = Allocate(settings.desktopCount * sizeof(char*));
      for(x = 0; x < settings.desktopCount; x++) {
         desktopNames[x] = NULL;
      }
   }
   for(x = 0; x < settings.desktopCount; x++) {
      if(desktopNames[x] == NULL) {
         desktopNames[x] = Allocate(4 * sizeof(char));
         snprintf(desktopNames[x], 4, "%d", x + 1);
      }
   }

   showingDesktop = 0;

}

/** Release desktop data. */
void DestroyDesktops() {

   unsigned int x;

   if(desktopNames) {
      for(x = 0; x < settings.desktopCount; x++) {
         Release(desktopNames[x]);
      }
      Release(desktopNames);
      desktopNames = NULL;
   }

}

/** Change to the desktop to the right. */
char RightDesktop()
{
   int x, y;
   if(settings.desktopWidth > 1) {
      y = currentDesktop / settings.desktopWidth;
      x = (currentDesktop + 1) % settings.desktopWidth;
      ChangeDesktop(y * settings.desktopWidth + x);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the desktop to the left. */
char LeftDesktop()
{
   int x, y;
   if(settings.desktopWidth > 1) {
      y = currentDesktop / settings.desktopWidth;
      x = currentDesktop % settings.desktopWidth;
      x = x > 0 ? x - 1 : settings.desktopWidth - 1;
      ChangeDesktop(y * settings.desktopWidth + x);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the desktop above. */
char AboveDesktop()
{
   unsigned int next;
   if(settings.desktopHeight > 1) {
      if(currentDesktop >= settings.desktopWidth) {
         next = currentDesktop - settings.desktopWidth;
      } else {
         next = currentDesktop
              + (settings.desktopHeight - 1) * settings.desktopWidth;
      }
      ChangeDesktop(next);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the desktop below. */
char BelowDesktop()
{
   unsigned int next;
   if(settings.desktopHeight > 1) {
      next = (currentDesktop + settings.desktopWidth) % settings.desktopCount;
      ChangeDesktop(next);
      return 1;
   } else {
      return 0;
   }
}

/** Change to the specified desktop. */
void ChangeDesktop(unsigned int desktop)
{

   ClientNode *np;
   unsigned int x;

   if(JUNLIKELY(desktop >= settings.desktopCount)) {
      return;
   }

   if(currentDesktop == desktop) {
      return;
   }

   /* Hide clients from the old desktop.
    * Note that we show clients in a separate loop to prevent an issue
    * with clients losing focus.
    */
   for(x = 0; x < LAYER_COUNT; x++) {
      for(np = nodes[x]; np; np = np->next) {
         if(np->state.status & STAT_STICKY) {
            continue;
         }
         if(np->state.desktop == currentDesktop) {
            HideClient(np);
         }
      }
   }

   /* Show clients on the new desktop. */
   for(x = 0; x < LAYER_COUNT; x++) {
      for(np = nodes[x]; np; np = np->next) {
         if(np->state.status & STAT_STICKY) {
            continue;
         }
         if(np->state.desktop == desktop) {
            ShowClient(np);
         }
      }
   }

   currentDesktop = desktop;

   SetCardinalAtom(rootWindow, ATOM_NET_CURRENT_DESKTOP, currentDesktop);

   RestackClients();
   UpdateTaskBar();

   LoadBackground(desktop);

}

/** Create a desktop menu. */
Menu *CreateDesktopMenu(unsigned int mask)
{

   Menu *menu;
   MenuItem *item;
   int x;

   menu = Allocate(sizeof(Menu));
   menu->itemHeight = 0;
   menu->items = NULL;
   menu->label = NULL;

   for(x = settings.desktopCount - 1; x >= 0; x--) {

      item = Allocate(sizeof(MenuItem));
      item->type = MENU_ITEM_NORMAL;
      item->iconName = NULL;
      item->submenu = NULL;
      item->next = menu->items;
      menu->items = item;

      item->action.type = MA_DESKTOP;
      item->action.data.i = x;

      item->name = Allocate(strlen(desktopNames[x]) + 3);
      if(mask & (1 << x)) {
         strcpy(item->name, "[");
         strcat(item->name, desktopNames[x]);
         strcat(item->name, "]");
      } else {
         strcpy(item->name, " ");
         strcat(item->name, desktopNames[x]);
         strcat(item->name, " ");
      }

   }

   return menu;

}

/** Toggle the "show desktop" state. */
void ShowDesktop()
{

   ClientNode *np;
   int layer;

   for(layer = 0; layer < LAYER_COUNT; layer++) {
      for(np = nodes[layer]; np; np = np->next) {
         if(!(np->state.status & STAT_NOLIST)) {
            if(showingDesktop) {
               if(np->state.status & STAT_SDESKTOP) {
                  RestoreClient(np, 0);
               }
            } else if(np->state.desktop == currentDesktop
                || (np->state.status & STAT_STICKY)) {
               if(np->state.status & (STAT_MAPPED | STAT_SHADED)) {
                  MinimizeClient(np, 0);
                  np->state.status |= STAT_SDESKTOP;
               }
            }
         }
      }
   }

   showingDesktop = !showingDesktop;
   SetCardinalAtom(rootWindow, ATOM_NET_SHOWING_DESKTOP, showingDesktop);

   RestackClients();

}

/** Set the name for a desktop. */
void SetDesktopName(unsigned int desktop, const char *str)
{

   unsigned int x;

   if(JUNLIKELY(!str)) {
      Warning(_("empty Desktops Name tag"));
      return;
   }

   Assert(desktop >= 0);
   Assert(desktop < settings.desktopWidth * settings.desktopHeight);

   if(!desktopNames) {
      desktopNames = Allocate(settings.desktopCount * sizeof(char*));
      for(x = 0; x < settings.desktopCount; x++) {
         desktopNames[x] = NULL;
      }
   }

   Assert(desktopNames[desktop] == NULL);

   desktopNames[desktop] = CopyString(str);

}

/** Get the name of a desktop. */
const char *GetDesktopName(unsigned int desktop)
{
   Assert(desktop < settings.desktopCount);
   if(desktopNames && desktopNames[desktop]) {
      return desktopNames[desktop];
   } else {
      return "";
   }
}

