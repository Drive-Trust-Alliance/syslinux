/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2006 H. Peter Anvin - All Rights Reserved
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, and to permit persons to whom
 *   the Software is furnished to do so, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------- */

#include <inttypes.h>
#include <colortbl.h>
#include "vesa.h"
#include "video.h"

/*
 * Visible cursor information
 */
static uint8_t cursor_pattern[FONT_MAX_HEIGHT];
static struct vesa_char *cursor_pointer = NULL;
static int cursor_x, cursor_y;

/*
 * Linear alpha blending.  Useless for anything that's actually
 * depends on color accuracy (because of gamma), but it's fine for
 * what we want.
 *
 * This algorithm is exactly equivalent to (alpha*fg+(255-alpha)*bg)/255
 * for all 8-bit values, but is substantially faster.
 */
static inline __attribute__((always_inline))
  uint8_t alpha_val(uint8_t fg, uint8_t bg, uint8_t alpha)
{
  unsigned int tmp = 1 + alpha*fg + (255-alpha)*bg;
  return (tmp + (tmp >> 8)) >> 8;
}

static uint32_t alpha_pixel(uint32_t fg, uint32_t bg)
{
  uint8_t alpha = fg >> 24;
  uint8_t fg_r = fg >> 16;
  uint8_t fg_g = fg >> 8;
  uint8_t fg_b = fg;
  uint8_t bg_r = bg >> 16;
  uint8_t bg_g = bg >> 8;
  uint8_t bg_b = bg;

  return
    (alpha_val(fg_r, bg_r, alpha) << 16)|
    (alpha_val(fg_g, bg_g, alpha) << 8)|
    (alpha_val(fg_b, bg_b, alpha));
}

static void vesacon_update_characters(int row, int col, int nrows, int ncols)
{
  const int height = __vesacon_font_height;
  const int width = FONT_WIDTH;
  uint32_t *bgrowptr, *bgptr, *fbrowptr, *fbptr, bgval, fgval;
  uint32_t fgcolor = 0, bgcolor = 0, color;
  uint8_t chbits = 0, chxbits = 0, chsbits = 0;
  int i, j, pixrow, pixsrow;
  struct vesa_char *rowptr, *rowsptr, *cptr, *csptr;

  bgrowptr  = &__vesacon_background[row*height+VIDEO_BORDER][col*width+VIDEO_BORDER];
  fbrowptr = ((uint32_t *)__vesa_info.mi.lfb_ptr)+
    ((row*height+VIDEO_BORDER)*VIDEO_X_SIZE)+(col*width+VIDEO_BORDER);

  /* Note that we keep a 1-character guard area around the real text area... */
  rowptr  = &__vesacon_text_display[(row+1)*(TEXT_PIXEL_COLS/FONT_WIDTH+2)+(col+1)];
  rowsptr = rowptr - ((TEXT_PIXEL_COLS/FONT_WIDTH+2)+1);
  pixrow = 0;
  pixsrow = height-1;

  for (i = height*nrows; i >= 0; i--) {
    bgptr = bgrowptr;
    fbptr = fbrowptr;

    cptr = rowptr;
    csptr = rowsptr;

    chsbits = __vesacon_graphics_font[csptr->ch][pixsrow];
    if (__unlikely(csptr == cursor_pointer))
      chsbits |= cursor_pattern[pixsrow];
    chsbits &= (csptr->sha & 0x02) ? 0xff : 0x00;
    chsbits ^= (csptr->sha & 0x01) ? 0xff : 0x00;
    chsbits <<= 6;
    csptr++;

    for (j = width*ncols; j >= 0; j--) {
      chbits <<= 1;
      chsbits <<= 1;
      chxbits <<= 1;

      switch (j % FONT_WIDTH) {
      case 0:
	chbits = __vesacon_graphics_font[cptr->ch][pixrow];
	if (__unlikely(cptr == cursor_pointer))
	  chbits |= cursor_pattern[pixrow];
	chxbits = chbits;
	chxbits &= (cptr->sha & 0x02) ? 0xff : 0x00;
	chxbits ^= (cptr->sha & 0x01) ? 0xff : 0x00;
	fgcolor = console_color_table[cptr->attr].argb_fg;
	bgcolor = console_color_table[cptr->attr].argb_bg;
	cptr++;
	break;
      case FONT_WIDTH-1:
	chsbits = __vesacon_graphics_font[csptr->ch][pixsrow];
	if (__unlikely(csptr == cursor_pointer))
	  chsbits |= cursor_pattern[pixsrow];
	chsbits &= (csptr->sha & 0x02) ? 0xff : 0x00;
	chsbits ^= (csptr->sha & 0x01) ? 0xff : 0x00;
	csptr++;
	break;
      default:
	break;
      }

      /* If this pixel is raised, use the offsetted value */
      bgval = (chxbits & 0x80) ? bgptr[VIDEO_X_SIZE+1] : *bgptr;
      bgptr++;

      /* If this pixel is set, use the fg color, else the bg color */
      fgval = (chbits & 0x80) ? fgcolor : bgcolor;

      /* Produce the combined color pixel value */
      color = alpha_pixel(fgval, bgval);

      /* Apply the shadow (75% shadow) */
      if ((chsbits & ~chxbits) & 0x80) {
	color >>= 2;
	color &= 0x3f3f3f;
      }

      *fbptr++ = color;
    }

    bgrowptr += VIDEO_X_SIZE;
    fbrowptr += VIDEO_X_SIZE;

    if (++pixrow == height) {
      rowptr += TEXT_PIXEL_COLS/FONT_WIDTH+2;
      pixrow = 0;
    }
    if (++pixsrow == height) {
      rowsptr += TEXT_PIXEL_COLS/FONT_WIDTH+2;
      pixsrow = 0;
    }
  }
}

/* Bounding box for changed text.  The (x1, y1) coordinates are +1! */
static unsigned int upd_x0 = -1U, upd_x1, upd_y0 = -1U, upd_y1;

/* Update the range already touched by various variables */
void __vesacon_doit(void)
{
  if (upd_x1 > upd_x0 && upd_y1 > upd_y0) {
    vesacon_update_characters(upd_y0, upd_x0, upd_y1-upd_y0, upd_x1-upd_x0);
    upd_x0 = upd_y0 = -1U;
    upd_x1 = upd_y1 = 0;
  }
}

/* Mark a range for update; note argument sequence is the same as
   vesacon_update_characters() */
static inline void vesacon_touch(int row, int col, int rows, int cols)
{
  unsigned int y0 = row;
  unsigned int x0 = col;
  unsigned int y1 = y0+rows;
  unsigned int x1 = x0+cols;

  if (y0 < upd_y0)
    upd_y0 = y0;
  if (y1 > upd_y1)
    upd_y1 = y1;
  if (x0 < upd_x0)
    upd_x0 = x0;
  if (x1 > upd_x1)
    upd_x1 = x1;
}

/* Fill a number of characters... */
static inline struct vesa_char *vesacon_fill(struct vesa_char *ptr,
					     struct vesa_char fill,
					     unsigned int count)
{
  asm volatile("cld; rep; stosl"
	       : "+D" (ptr), "+c" (count)
	       : "a" (fill)
	       : "memory");

  return ptr;
}

/* Erase a region of the screen */
void __vesacon_erase(int x0, int y0, int x1, int y1, uint8_t attr, int rev)
{
  int y;
  struct vesa_char *ptr = &__vesacon_text_display
    [(y0+1)*(TEXT_PIXEL_COLS/FONT_WIDTH+2)+(x0+1)];
  struct vesa_char fill = {
    .ch = ' ',
    .attr = attr,
    .sha = rev
  };
  int ncols = x1-x0+1;

  for (y = y0; y <= y1; y++) {
    vesacon_fill(ptr, fill, ncols);
    ptr += TEXT_PIXEL_COLS/FONT_WIDTH+2;
  }

  vesacon_touch(y0, x0, y1-y0+1, ncols);
}

/* Scroll the screen up */
void __vesacon_scroll_up(int nrows, uint8_t attr, int rev)
{
  struct vesa_char *fromptr = &__vesacon_text_display
    [(nrows+1)*(TEXT_PIXEL_COLS/FONT_WIDTH+2)];
  struct vesa_char *toptr = &__vesacon_text_display
    [(TEXT_PIXEL_COLS/FONT_WIDTH+2)];
  int dword_count = (__vesacon_text_rows-nrows)*(TEXT_PIXEL_COLS/FONT_WIDTH+2);
  struct vesa_char fill = {
    .ch   = ' ',
    .attr = attr,
    .sha  = rev,
  };

  asm volatile("cld ; rep ; movsl"
	       : "+D" (toptr), "+S" (fromptr), "+c" (dword_count));

  dword_count = nrows*(TEXT_PIXEL_COLS/FONT_WIDTH+2);

  /* Danger, Will Robinson: this is wrong if rev != SHADOW_NORMAL */
  vesacon_fill(toptr, fill, dword_count);

  vesacon_touch(0, 0, __vesacon_text_rows, TEXT_PIXEL_COLS/FONT_WIDTH);
}

/* Draw text at a specific area of the screen */
void __vesacon_write_at(int x, int y, const char *str,
			uint8_t attr, int rev)
{
  int n = 0;
  struct vesa_char *ptr = &__vesacon_text_display
    [(y+1)*(TEXT_PIXEL_COLS/FONT_WIDTH+2)+(x+1)];

  while (*str) {
    ptr->ch   = *str;
    ptr->attr = attr;
    ptr->sha  = rev;

    n++;
    str++;
    ptr++;
  }

  vesacon_touch(y, x, 1, n);
}

/* Draw one character text at a specific area of the screen */
void __vesacon_write_char(int x, int y, uint8_t ch, uint8_t attr, int rev)
{
  struct vesa_char *ptr = &__vesacon_text_display
    [(y+1)*(TEXT_PIXEL_COLS/FONT_WIDTH+2)+(x+1)];

  ptr->ch   = ch;
  ptr->attr = attr;
  ptr->sha  = rev;

  vesacon_touch(y, x, 1, 1);
}

void __vesacon_set_cursor(int x, int y, int visible)
{
  struct vesa_char *ptr = &__vesacon_text_display
    [(y+1)*(TEXT_PIXEL_COLS/FONT_WIDTH+2)+(x+1)];

  if (cursor_pointer)
    vesacon_touch(cursor_y, cursor_x, 1, 1);

  if (!visible) {
    /* Invisible cursor */
    cursor_pointer = NULL;
  } else {
    cursor_pointer = ptr;
    vesacon_touch(y, x, 1, 1);
  }

  cursor_x = x;
  cursor_y = y;
}

void __vesacon_init_cursor(int font_height)
{
  int r0 = font_height - (font_height < 10 ? 2 : 3);

  if (r0 < 0)
    r0 = 0;

  /* cursor_pattern is assumed to be all zero */
  cursor_pattern[r0] = 0xff;
  cursor_pattern[r0+1] = 0xff;
}
