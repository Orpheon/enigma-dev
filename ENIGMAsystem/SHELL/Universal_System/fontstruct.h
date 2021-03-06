/********************************************************************************\
**                                                                              **
**  Copyright (C) 2008 Josh Ventura                                             **
**  Copyright (C) 2010 Alasdair Morrison <tgmg@g-java.com>                      **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/

#ifndef _FONTSTRUCT__H
#define _FONTSTRUCT__H

namespace enigma
{
  struct fontglyph
  {
    int   x,  y,  x2,  y2; // Draw coordinates, relative to the top-left corner of a full glyph. Added to xx and yy for draw.
    float tx, ty, tx2, ty2; // Texture coords: used to locate glyph on bound font texture
    float xs; // Spacing: used to increment xx
  };
  struct font
  {
    // Trivia
    string name, fontname;
    int fontsize; bool bold, italic;

    // Metrics and such
    unsigned int glyphstart, glyphcount;
    fontglyph *glyphs;
    unsigned int height, yoffset;

    // Texture layer
    unsigned int texture;
    int twid, thgt;
  };
  struct rawfont {
    string name;
    int id;

    string fontname;
    int fontsize; bool bold, italic;
    unsigned int glyphstart, glyphcount;
  };
  extern rawfont rawfontdata[];
  extern font **fontstructarray;

  extern int rawfontcount, rawfontmaxid;
  int font_new(unsigned char gs, unsigned char gc); // Creates a new font, allocating 'gc' glyphs
  int font_pack(enigma::font *font, int spr, unsigned char gcount, bool prop, int sep);
}

int font_add(string name, int size, bool bold, bool italic, unsigned char first, unsigned char last);
bool font_replace(int ind, string name, int size, bool bold, bool italic, unsigned char first, unsigned char last);
int  font_add_sprite(int spr, unsigned char first, bool prop, int sep);
bool font_replace_sprite(int ind, int spr, unsigned char first, bool prop, int sep);
bool font_get_bold(int fnt);
bool font_get_italic(int fnt);
unsigned char font_get_first(int fnt);
unsigned char font_get_last(int fnt);
string font_get_fontname(int fnt);
void font_delete(int fnt);
bool font_exists(int fnt);

#endif
