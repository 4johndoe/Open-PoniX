/**
 * @file font.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for the font functions.
 *
 */

#ifndef FONT_H
#define FONT_H

#include "color.h"

/** Enumeration of different components that use fonts. */
typedef unsigned char FontType;
#define FONT_BORDER     0
#define FONT_MENU       1
#define FONT_TASK       2
#define FONT_POPUP      3
#define FONT_CLOCK      4
#define FONT_TRAY       5
#define FONT_TRAYBUTTON 6
#define FONT_PAGER      7
#define FONT_COUNT      8

void InitializeFonts();
void StartupFonts();
void ShutdownFonts();
void DestroyFonts();

/** Set the font to use for a component.
 * @param type The font component.
 * @param value The font to use.
 */
void SetFont(FontType type, const char *value);

/** Render a string.
 * @param d The drawable on which to render the string.
 * @param font The font to use.
 * @param color The text color to use.
 * @param x The x-coordinate at which to render.
 * @param y The y-coordinate at which to render.
 * @param width The maximum width allowed.
 * @param str The string to render.
 */
void RenderString(Drawable d, FontType font, ColorType color,
                  int x, int y, int width, const char *str);

/** Get the width of a string.
 * @param ft The font used to determine the width.
 * @param str The string whose width to get.
 * @return The width of the string in pixels.
 */
int GetStringWidth(FontType ft, const char *str);

/** Get the height of a string.
 * @param ft The font used to determine the height.
 * @return The height in pixels.
 */
int GetStringHeight(FontType ft);

#endif /* FONT_H */

