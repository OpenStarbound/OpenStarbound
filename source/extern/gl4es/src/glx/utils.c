#include "utils.h"
/*
 * Generate OpenGL-compatible bitmap.
 * From Mesa-9.0.1
 */
#ifndef NOX11

void
fill_bitmap(Display * dpy, Window win, GC gc,
            unsigned int width, unsigned int height,
            int x0, int y0, unsigned int c, GLubyte * bitmap)
{
   XImage *image;
   unsigned int x, y;
   Pixmap pixmap;
   XChar2b char2b;

   pixmap = XCreatePixmap(dpy, win, 8 * width, height, 1);
   XSetForeground(dpy, gc, 0);
   XFillRectangle(dpy, pixmap, gc, 0, 0, 8 * width, height);
   XSetForeground(dpy, gc, 1);

   char2b.byte1 = (c >> 8) & 0xff;
   char2b.byte2 = (c & 0xff);

   XDrawString16(dpy, pixmap, gc, x0, y0, &char2b, 1);

   image = XGetImage(dpy, pixmap, 0, 0, 8 * width, height, 1, XYPixmap);
   if (image) {
      /* Fill the bitmap (X11 and OpenGL are upside down wrt each other).  */
      for (y = 0; y < height; y++)
         for (x = 0; x < 8 * width; x++)
            if (XGetPixel(image, x, y))
               bitmap[width * (height - y - 1) + x / 8] |=
                  (1 << (7 - (x % 8)));
      XDestroyImage(image);
   }

   XFreePixmap(dpy, pixmap);
}
/*
 * determine if a given glyph is valid and return the
 * corresponding XCharStruct.
 * From MesaGL-9.0.1
 */
XCharStruct *
isvalid(XFontStruct * fs, int which)
{
   unsigned int rows, pages;
   int byte1 = 0, byte2 = 0;
   int i, valid = 1;

   rows = fs->max_byte1 - fs->min_byte1 + 1;
   pages = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;

   if (rows == 1) {
      /* "linear" fonts */
      if ((fs->min_char_or_byte2 > which) || (fs->max_char_or_byte2 < which))
         valid = 0;
   }
   else {
      /* "matrix" fonts */
      byte2 = which & 0xff;
      byte1 = which >> 8;
      if ((fs->min_char_or_byte2 > byte2) ||
          (fs->max_char_or_byte2 < byte2) ||
          (fs->min_byte1 > byte1) || (fs->max_byte1 < byte1))
         valid = 0;
   }

   if (valid) {
      if (fs->per_char) {
         if (rows == 1) {
            /* "linear" fonts */
            return (fs->per_char + (which - fs->min_char_or_byte2));
         }
         else {
            /* "matrix" fonts */
            i = ((byte1 - fs->min_byte1) * pages) +
               (byte2 - fs->min_char_or_byte2);
            return (fs->per_char + i);
         }
      }
      else {
         return (&fs->min_bounds);
      }
   }
   return (NULL);
}

#endif //NOX11