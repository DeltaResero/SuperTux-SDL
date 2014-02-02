//  $Id$
//
//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//  02111-1307, USA.

#ifndef __TEXTSCROLLER_H__
#define __TEXTSCROLLER_H__

#include <vector>
#include <string>
#include <map>

#include "screen.hpp"
#include "math/vector.hpp"
#include "math/rect.hpp"

class DrawingContext;
class Surface;
class Font;

/**
 * Helper class for InfoBox: Represents a line of text
 */
class InfoBoxLine
{
public:
  enum LineType { NORMAL, NORMAL_LEFT, SMALL, HEADING, REFERENCE, IMAGE};

  InfoBoxLine(char format_char, const std::string& text);
  ~InfoBoxLine();

  void draw(DrawingContext& context, const Rect& bbox, int layer);
  float get_height();

  static const std::vector<InfoBoxLine*> split(const std::string& text, float width);

private:
  InfoBoxLine::LineType lineType;
  Font* font;
  std::string text;
  Surface* image;
};

/** This class is displaying a box with information text inside the game
 */
class InfoBox
{
public:
  InfoBox(const std::string& text);
  ~InfoBox();

  void draw(DrawingContext& context);
  void scrolldown();
  void scrollup();
  void pagedown();
  void pageup();

private:
  size_t firstline;
  std::vector<InfoBoxLine*> lines;
  std::map<std::string, Surface*> images;
  Surface* arrow_scrollup;
  Surface* arrow_scrolldown;
};

/**
 * Screen that displays intro text, extro text, etc.
 */
class TextScroller : public Screen
{
public:
  TextScroller(const std::string& file);
  virtual ~TextScroller();

  void setup();
  void draw(DrawingContext& context);
  void update(float elapsed_time);

private:
  float defaultspeed;
  float speed;
  std::string music;
  std::auto_ptr<Surface> background;
  std::vector<InfoBoxLine*> lines;
  float scroll;
  bool fading;
};

#endif
