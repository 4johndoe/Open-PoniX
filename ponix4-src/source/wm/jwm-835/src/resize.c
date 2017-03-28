/**
 * @file resize.c
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Functions to handle resizing client windows.
 *
 */

#include "jwm.h"
#include "resize.h"
#include "client.h"
#include "outline.h"
#include "main.h"
#include "cursor.h"
#include "misc.h"
#include "pager.h"
#include "status.h"
#include "key.h"
#include "event.h"
#include "border.h"
#include "settings.h"

static char shouldStopResize;

static void StopResize(ClientNode *np);
static void ResizeController(int wasDestroyed);
static void FixWidth(ClientNode *np);
static void FixHeight(ClientNode *np);

/** Callback to stop a resize. */
void ResizeController(int wasDestroyed)
{
   if(settings.resizeMode == RESIZE_OUTLINE) {
      ClearOutline();
   }
   JXUngrabPointer(display, CurrentTime);
   JXUngrabKeyboard(display, CurrentTime);

/* harry */
/*
   DestroyResizeWindow();
*/
   shouldStopResize = 1;
}

/** Resize a client window (mouse initiated). */
void ResizeClient(ClientNode *np, BorderActionType action,
                  int startx, int starty)
{

   XEvent event;
   int oldx, oldy;
   int oldw, oldh;
   int gwidth, gheight;
   int lastgwidth, lastgheight;
   int delta;
   int north, south, east, west;
   int ratio, minr, maxr;

   Assert(np);
   Assert(np->state.border & BORDER_RESIZE);

   if(JUNLIKELY(!GrabMouseForResize(action))) {
      Debug("ResizeClient: could not grab mouse");
      return;
   }

   np->controller = ResizeController;
   shouldStopResize = 0;

   oldx = np->x;
   oldy = np->y;
   oldw = np->width;
   oldh = np->height;

   gwidth = (np->width - np->baseWidth) / np->xinc;
   gheight = (np->height - np->baseHeight) / np->yinc;

   GetBorderSize(&np->state, &north, &south, &east, &west);

   startx += np->x - west;
   starty += np->y - north;

/* harry */
/*
   CreateResizeWindow(np);
   UpdateResizeWindow(np, gwidth, gheight);
*/

   if(!(GetMouseMask() & (Button1Mask | Button3Mask))) {
      StopResize(np);
      return;
   }

   for(;;) {

      WaitForEvent(&event);

      if(shouldStopResize) {
         np->controller = NULL;
         return;
      }

      switch(event.type) {
      case ButtonRelease:
         if(   event.xbutton.button == Button1
            || event.xbutton.button == Button3) {
            StopResize(np);
            return;
         }
         break;
      case MotionNotify:

         SetMousePosition(event.xmotion.x_root, event.xmotion.y_root);
         DiscardMotionEvents(&event, np->window);

         if(action & BA_RESIZE_N) {
            delta = (event.xmotion.y - starty) / np->yinc;
            delta *= np->yinc;
            if(oldh - delta >= np->minHeight
               && (oldh - delta <= np->maxHeight || delta > 0)) {
               np->height = oldh - delta;
               np->y = oldy + delta;
            }
            if(!(action & (BA_RESIZE_E | BA_RESIZE_W))) {
               FixWidth(np);
            }
         }
         if(action & BA_RESIZE_S) {
            delta = (event.xmotion.y - starty) / np->yinc;
            delta *= np->yinc;
            np->height = oldh + delta;
            np->height = Max(np->height, np->minHeight);
            np->height = Min(np->height, np->maxHeight);
            if(!(action & (BA_RESIZE_E | BA_RESIZE_W))) {
               FixWidth(np);
            }
         }
         if(action & BA_RESIZE_E) {
            delta = (event.xmotion.x - startx) / np->xinc;
            delta *= np->xinc;
            np->width = oldw + delta;
            np->width = Max(np->width, np->minWidth);
            np->width = Min(np->width, np->maxWidth);
            if(!(action & (BA_RESIZE_N | BA_RESIZE_S))) {
               FixHeight(np);
            }
         }
         if(action & BA_RESIZE_W) {
            delta = (event.xmotion.x - startx) / np->xinc;
            delta *= np->xinc;
            if(oldw - delta >= np->minWidth
               && (oldw - delta <= np->maxWidth || delta > 0)) {
               np->width = oldw - delta;
               np->x = oldx + delta;
            }
            if(!(action & (BA_RESIZE_N | BA_RESIZE_S))) {
               FixHeight(np);
            }
         }

         if(np->sizeFlags & PAspect) {
            if((action & (BA_RESIZE_N | BA_RESIZE_S)) &&
               (action & (BA_RESIZE_E | BA_RESIZE_W))) {

               /* Fixed point with a 16-bit fraction. */
               ratio = (np->width << 16) / np->height;

               minr = (np->aspect.minx << 16) / np->aspect.miny;
               if(ratio < minr) {
                  delta = np->width;
                  np->width = (np->height * minr) >> 16;
                  if(action & BA_RESIZE_W) {
                     np->x -= np->width - delta;
                  }
               }

               maxr = (np->aspect.maxx << 16) / np->aspect.maxy;
               if(ratio > maxr) {
                  delta = np->height;
                  np->height = (np->width << 16) / maxr;
                  if(action & BA_RESIZE_N) {
                     np->y -= np->height - delta;
                  }
               }

            }
         }

         lastgwidth = gwidth;
         lastgheight = gheight;

         gwidth = (np->width - np->baseWidth) / np->xinc;
         gheight = (np->height - np->baseHeight) / np->yinc;

         if(lastgheight != gheight || lastgwidth != gwidth) {

/* harry */
/*
            UpdateResizeWindow(np, gwidth, gheight);
*/
            if(settings.resizeMode == RESIZE_OUTLINE) {
               ClearOutline();
               if(np->state.status & STAT_SHADED) {
                  DrawOutline(np->x - west, np->y - north,
                     np->width + west + east, north + south);
               } else {
                  DrawOutline(np->x - west, np->y - north,
                     np->width + west + east,
                     np->height + north + south);
               }
            } else {
               ResetBorder(np);
               SendConfigureEvent(np);
            }

            UpdatePager();

         }

         break;
      default:
         break;
      }
   }

}

/** Resize a client window (keyboard or menu initiated). */
void ResizeClientKeyboard(ClientNode *np)
{

   XEvent event;
   int gwidth, gheight;
   int lastgwidth, lastgheight;
   int north, south, east, west;
   int deltax, deltay;
   int ratio, minr, maxr;

   Assert(np);

   if(!(np->state.border & BORDER_RESIZE)) {
      return;
   }
   if(np->state.status & (STAT_VMAX | STAT_HMAX)) {
      return;
   }

   if(JUNLIKELY(JXGrabKeyboard(display, np->parent, True, GrabModeAsync,
                               GrabModeAsync, CurrentTime) != GrabSuccess)) {
      Debug("ResizeClient: could not grab keyboard");
      return;
   }
   GrabMouseForResize(BA_RESIZE_S | BA_RESIZE_E | BA_RESIZE);

   np->controller = ResizeController;
   shouldStopResize = 0;

   gwidth = (np->width - np->baseWidth) / np->xinc;
   gheight = (np->height - np->baseHeight) / np->yinc;

   GetBorderSize(&np->state, &north, &south, &east, &west);

/* harry */
/*
   CreateResizeWindow(np);
   UpdateResizeWindow(np, gwidth, gheight);
*/

   if(np->state.status & STAT_SHADED) {
      MoveMouse(rootWindow, np->x + np->width, np->y);
   } else {
      MoveMouse(rootWindow, np->x + np->width, np->y + np->height);
   }
   DiscardMotionEvents(&event, np->window);

   for(;;) {

      WaitForEvent(&event);

      if(shouldStopResize) {
         np->controller = NULL;
         return;
      }

      deltax = 0;
      deltay = 0;

      if(event.type == KeyPress) {

         DiscardKeyEvents(&event, np->window);
         switch(GetKey(&event.xkey) & 0xFF) {
         case KEY_UP:
            deltay = Min(-np->yinc, -10);
            break;
         case KEY_DOWN:
            deltay = Max(np->yinc, 10);
            break;
         case KEY_RIGHT:
            deltax = Max(np->xinc, 10);
            break;
         case KEY_LEFT:
            deltax = Min(-np->xinc, -10);
            break;
         default:
            StopResize(np);
            return;
         }

      } else if(event.type == MotionNotify) {

         SetMousePosition(event.xmotion.x_root, event.xmotion.y_root);
         DiscardMotionEvents(&event, np->window);

         deltax = event.xmotion.x - (np->x + np->width);
         if(np->state.status & STAT_SHADED) {
            deltay = 0;
         } else {
            deltay = event.xmotion.y - (np->y + np->height);
         }

      } else if(event.type == ButtonRelease) {

         StopResize(np);
         return;

      }

      if(abs(deltax) < np->xinc && abs(deltay) < np->yinc) {
         continue;
      }

      deltay -= deltay % np->yinc;
      np->height += deltay;
      np->height = Max(np->height, np->minHeight);
      np->height = Min(np->height, np->maxHeight);
      deltax -= deltax % np->xinc;
      np->width += deltax;
      np->width = Max(np->width, np->minWidth);
      np->width = Min(np->width, np->maxWidth);

      if(np->sizeFlags & PAspect) {

         ratio = (np->width << 16) / np->height;

         minr = (np->aspect.minx << 16) / np->aspect.miny;
         if(ratio < minr) {
            np->width = (np->height * minr) >> 16;
         }

         maxr = (np->aspect.maxx << 16) / np->aspect.maxy;
         if(ratio > maxr) {
            np->height = (np->width << 16) / maxr;
         }

      }

      lastgwidth = gwidth;
      lastgheight = gheight;
      gwidth = (np->width - np->baseWidth) / np->xinc;
      gheight = (np->height - np->baseHeight) / np->yinc;

      if(lastgwidth != gwidth || lastgheight != gheight) {

/* harry */
/*
         UpdateResizeWindow(np, gwidth, gheight);
*/
         if(settings.resizeMode == RESIZE_OUTLINE) {
            ClearOutline();
            if(np->state.status & STAT_SHADED) {
               DrawOutline(np->x - west, np->y - north,
                  np->width + west + east,
                  north + south);
            } else {
               DrawOutline(np->x - west, np->y - north,
                  np->width + west + east,
                  np->height + north + south);
            }
         } else {
            ResetBorder(np);
            SendConfigureEvent(np);
         }

         UpdatePager();

      }

   }

}

/** Stop a resize action. */
void StopResize(ClientNode *np)
{

   np->controller = NULL;

   /* Set the old width/height if maximized so the window
    * is restored to the new size. */
   if(np->state.status & STAT_VMAX) {
      np->oldWidth = np->width;
      np->oldx = np->x;
   }
   if(np->state.status & STAT_HMAX) {
      np->oldHeight = np->height;
      np->oldy = np->y;
   }

   if(settings.resizeMode == RESIZE_OUTLINE) {
      ClearOutline();
   }

   JXUngrabPointer(display, CurrentTime);
   JXUngrabKeyboard(display, CurrentTime);

/* harry */
/*
   DestroyResizeWindow();
*/

   ResetBorder(np);
   SendConfigureEvent(np);

}

/** Fix the width to match the aspect ratio. */
void FixWidth(ClientNode *np)
{

   int ratio, minr, maxr;

   Assert(np);

   if((np->sizeFlags & PAspect) && np->height > 0) {

      /* Fixed point with a 16-bit fraction. */
      ratio = (np->width << 16) / np->height;

      minr = (np->aspect.minx << 16) / np->aspect.miny;
      if(ratio < minr) {
         np->width = (np->height * minr) >> 16;
      }

      maxr = (np->aspect.maxx << 16) / np->aspect.maxy;
      if(ratio > maxr) {
         np->width = (np->height * maxr) >> 16;
      }

   }

}

/** Fix the height to match the aspect ratio. */
void FixHeight(ClientNode *np)
{

   int ratio, minr, maxr;

   Assert(np);

   if((np->sizeFlags & PAspect) && np->height > 0) {

      ratio = (np->width << 16) / np->height;

      minr = (np->aspect.minx << 16) / np->aspect.miny;
      if(ratio < minr) {
         np->height = (np->width << 16) / minr;
      }

      maxr = (np->aspect.maxx << 16) / np->aspect.maxy;
      if(ratio > maxr) {
         np->height = (np->width << 16) / maxr;
      }

   }

}

