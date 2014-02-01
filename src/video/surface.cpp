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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <config.h>

#include <cassert>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <math.h>

#include <SDL.h>
#include <SDL_image.h>

#include "main.hpp"
#include "gameconfig.hpp"
#include "physfs/physfs_sdl.hpp"
#include "video/surface.hpp"
#include "video/drawing_context.hpp"
#include "video/color.hpp"
#include "image_texture.hpp"
#include "texture_manager.hpp"

Surface::Surface(const std::string& file)
{
  texture = texture_manager->get(file);
  texture->ref();

#ifdef HAVE_OPENGL
  use_opengl = config->use_opengl;

  if(use_opengl)
  {
    surface.opengl.uv_left = 0;
    surface.opengl.uv_top = 0;
    surface.opengl.uv_right = texture->get_uv_right();
    surface.opengl.uv_bottom = texture->get_uv_bottom();

    surface.opengl.width = texture->get_image_width();
    surface.opengl.height = texture->get_image_height();
  }
  else
#endif
  {
    memset(transforms, 0, NUM_EFFECTS * sizeof(SDL_Surface *));

    surface.sdl.offsetx = 0;
    surface.sdl.offsety = 0;
    surface.sdl.width = static_cast<int>(texture->get_image_width());
    surface.sdl.height = static_cast<int>(texture->get_image_height());

    surface.sdl.flipx = false;
  }
}

Surface::Surface(const std::string& file, int x, int y, int w, int h)
{
  texture = texture_manager->get(file);
  texture->ref();

#ifdef HAVE_OPENGL
  use_opengl = config->use_opengl;

  if(use_opengl)
  {
    float tex_w = static_cast<float> (texture->get_width());
    float tex_h = static_cast<float> (texture->get_height());
    surface.opengl.uv_left = static_cast<float>(x) / tex_w;
    surface.opengl.uv_top = static_cast<float>(y) / tex_h;
    surface.opengl.uv_right = static_cast<float>(x+w) / tex_w;
    surface.opengl.uv_bottom = static_cast<float>(y+h) / tex_h;

    surface.opengl.width = w;
    surface.opengl.height = h;
  }
  else
#endif
  {
    memset(transforms, 0, NUM_EFFECTS * sizeof(SDL_Surface *));

    surface.sdl.offsetx = x;
    surface.sdl.offsety = y;
    surface.sdl.width = w;
    surface.sdl.height = h;

    surface.sdl.flipx = false;
  }
}

Surface::Surface(const Surface& other)
{
  texture = other.texture;
  texture->ref();

#ifdef HAVE_OPENGL
  use_opengl = config->use_opengl;

  if(use_opengl)
  {
    surface.opengl.uv_left = other.surface.opengl.uv_left;
    surface.opengl.uv_top = other.surface.opengl.uv_top;
    surface.opengl.uv_right = other.surface.opengl.uv_right;
    surface.opengl.uv_bottom = other.surface.opengl.uv_bottom;
    surface.opengl.width = other.surface.opengl.width;
    surface.opengl.height = other.surface.opengl.height;
  }
  else
#endif
  {
    memset(transforms, 0, NUM_EFFECTS * sizeof(SDL_Surface *));

    surface.sdl.offsetx = other.surface.sdl.offsetx;
    surface.sdl.offsety = other.surface.sdl.offsety;
    surface.sdl.width = other.surface.sdl.width;
    surface.sdl.height = other.surface.sdl.height;

    surface.sdl.flipx = other.surface.sdl.flipx;
  }
}

const Surface&
Surface::operator= (const Surface& other)
{
  other.texture->ref();
  texture->unref();
  texture = other.texture;

#ifdef HAVE_OPENGL
  use_opengl = config->use_opengl;
 
  if(use_opengl)
  {
    surface.opengl.uv_left = other.surface.opengl.uv_left;
    surface.opengl.uv_top = other.surface.opengl.uv_top;
    surface.opengl.uv_right = other.surface.opengl.uv_right;
    surface.opengl.uv_bottom = other.surface.opengl.uv_bottom;
    surface.opengl.width = other.surface.opengl.width;
    surface.opengl.height = other.surface.opengl.height;
  }
  else
#endif
  {
    memset(transforms, 0, NUM_EFFECTS * sizeof(SDL_Surface *));

    surface.sdl.offsetx = other.surface.sdl.offsetx;
    surface.sdl.offsety = other.surface.sdl.offsety;
    surface.sdl.width = other.surface.sdl.width;
    surface.sdl.height = other.surface.sdl.height;

    surface.sdl.flipx = other.surface.sdl.flipx;
  }

  return *this;
}

Surface::~Surface()
{
  texture->unref();

#ifdef HAVE_OPENGL
  if(!use_opengl)
#endif
  {
    std::for_each(transforms, transforms + NUM_EFFECTS, SDL_FreeSurface);
  }
}

void
Surface::hflip()
{
#ifdef HAVE_OPENGL
  if(use_opengl)
  {
    std::swap(surface.opengl.uv_left, surface.opengl.uv_right);
  }
  else
#endif
  {
    surface.sdl.flipx = !surface.sdl.flipx;
  }
}

#ifdef HAVE_OPENGL
namespace
{
inline void intern_draw(float left, float top, float right, float bottom,                               float uv_left, float uv_top,
                               float uv_right, float uv_bottom,
                               DrawingEffect effect)
{
  if(effect & HORIZONTAL_FLIP)
    std::swap(uv_left, uv_right);
  if(effect & VERTICAL_FLIP) {
    std::swap(uv_top, uv_bottom);
  }

  glBegin(GL_QUADS);
  glTexCoord2f(uv_left, uv_top);
  glVertex2f(left, top);

  glTexCoord2f(uv_right, uv_top);
  glVertex2f(right, top);

  glTexCoord2f(uv_right, uv_bottom);
  glVertex2f(right, bottom);

  glTexCoord2f(uv_left, uv_bottom);
  glVertex2f(left, bottom);
  glEnd();
}

inline void intern_draw2(float left, float top, float right, float bottom,
                                float uv_left, float uv_top,
                                float uv_right, float uv_bottom,
                                float angle,
                                const Color& color,
                                const Blend& blend,
                                DrawingEffect effect)
{
  if(effect & HORIZONTAL_FLIP)
    std::swap(uv_left, uv_right);
  if(effect & VERTICAL_FLIP) {
    std::swap(uv_top, uv_bottom);
  }

  float center_x = (left + right) / 2;
  float center_y = (top + bottom) / 2;

  float sa = sinf(angle/180.0f*M_PI);
  float ca = cosf(angle/180.0f*M_PI);

  left  -= center_x;
  right -= center_x;

  top    -= center_y;
  bottom -= center_y;

  glBlendFunc(blend.sfactor, blend.dfactor);
  glColor4f(color.red, color.green, color.blue, color.alpha);
  glBegin(GL_QUADS);
  glTexCoord2f(uv_left, uv_top);
  glVertex2f(left*ca - top*sa + center_x,
             left*sa + top*ca + center_y);

  glTexCoord2f(uv_right, uv_top);
  glVertex2f(right*ca - top*sa + center_x,
             right*sa + top*ca + center_y);

  glTexCoord2f(uv_right, uv_bottom);
  glVertex2f(right*ca - bottom*sa + center_x,
             right*sa + bottom*ca + center_y);

  glTexCoord2f(uv_left, uv_bottom);
  glVertex2f(left*ca - bottom*sa + center_x,
             left*sa + bottom*ca + center_y);
  glEnd();

  // FIXME: find a better way to restore the blend mode
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
}
#endif

void
Surface::draw(float x, float y, float alpha,
#ifdef HAVE_OPENGL
              float angle, const Color& color, const Blend& blend,
#else
              float, const Color&, const Blend&,
#endif
              DrawingEffect effect) const
{
#ifdef HAVE_OPENGL
  if(use_opengl)
  {
    glBindTexture(GL_TEXTURE_2D, texture->get_handle());
    intern_draw2(x, y,
                 x + surface.opengl.width, y + surface.opengl.height,
                 surface.opengl.uv_left, surface.opengl.uv_top,
                 surface.opengl.uv_right, surface.opengl.uv_bottom,
                 angle,
                 color,
                 blend,
                 effect);
  }
  else
#endif

  {
    draw_part(0, 0, x, y, surface.sdl.width, surface.sdl.height, alpha, effect);
  }
}

void
Surface::draw(float x, float y, float alpha, DrawingEffect effect) const
{
#ifdef HAVE_OPENGL
if(use_opengl)
{
  glColor4f(1.0f, 1.0f, 1.0f, alpha);
  glBindTexture(GL_TEXTURE_2D, texture->get_handle());
  intern_draw(x, y,
              x + surface.opengl.width, y + surface.opengl.height,
              surface.opengl.uv_left, surface.opengl.uv_top,
              surface.opengl.uv_right, surface.opengl.uv_bottom, effect);
  glColor4f(1, 1, 1, 1);
}
else
#endif
{
  draw_part(0, 0, x, y, surface.sdl.width, surface.sdl.height, alpha, effect);
}
}

namespace
{
#define BILINEAR

#ifdef NAIVE
  SDL_Surface *scale(SDL_Surface *src, int numerator, int denominator)
  {
    if(numerator == denominator)
    {
      src->refcount++;
      return src;
    }
    else
    {
      SDL_Surface *dst = SDL_CreateRGBSurface(src->flags, src->w * numerator / denominator, src->h * numerator / denominator, src->format->BitsPerPixel, src->format->Rmask,  src->format->Gmask, src->format->Bmask, src->format->Amask);
      int bpp = dst->format->BytesPerPixel;
      for(int y = 0;y < dst->h;y++) {
        for(int x = 0;x < dst->w;x++) {
          Uint8 *srcpixel = (Uint8 *) src->pixels + (y * denominator / numerator) * src->pitch + (x * denominator / numerator) * bpp;
          Uint8 *dstpixel = (Uint8 *) dst->pixels + y * dst->pitch + x * bpp;
          switch(bpp) {
            case 4:
              dstpixel[3] = srcpixel[3];
            case 3:
              dstpixel[2] = srcpixel[2];
            case 2:
              dstpixel[1] = srcpixel[1];
            case 1:
              dstpixel[0] = srcpixel[0];
          }
        }
      }
      return dst;
    }
  }
#endif

#ifdef BILINEAR
  void getpixel(SDL_Surface *src, int srcx, int srcy, Uint8 color[4])
  {
    int bpp = src->format->BytesPerPixel;
    Uint8 *srcpixel = (Uint8 *) src->pixels + srcy * src->pitch + srcx * bpp;
    Uint32 mapped = 0;
    switch(bpp) {
      case 1:
        mapped = *srcpixel;
        break;
      case 2:
        mapped = *(Uint16 *)srcpixel;
        break;
      case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        mapped |= srcpixel[0] << 16;
        mapped |= srcpixel[1] << 8;
        mapped |= srcpixel[2] << 0;
#else
        mapped |= srcpixel[0] << 0;
        mapped |= srcpixel[1] << 8;
        mapped |= srcpixel[2] << 16;
#endif
        break;
      case 4:
        mapped = *(Uint32 *)srcpixel;
        break;
    }
    SDL_GetRGBA(mapped, src->format, &color[0], &color[1], &color[2], &color[3]);
  }

  void merge(Uint8 color[4], Uint8 color0[4], Uint8 color1[4], int rem, int total)
  {
    color[0] = (color0[0] * (total - rem) + color1[0] * rem) / total;
    color[1] = (color0[1] * (total - rem) + color1[1] * rem) / total;
    color[2] = (color0[2] * (total - rem) + color1[2] * rem) / total;
    color[3] = (color0[3] * (total - rem) + color1[3] * rem) / total;
  }

  SDL_Surface *scale(SDL_Surface *src, int numerator, int denominator)
  {
    if(numerator == denominator)
    {
      src->refcount++;
      return src;
    }
    else
    {
      SDL_Surface *dst = SDL_CreateRGBSurface(src->flags, src->w * numerator / denominator, src->h * numerator / denominator, src->format->BitsPerPixel, src->format->Rmask,  src->format->Gmask, src->format->Bmask, src->format->Amask);
      int bpp = dst->format->BytesPerPixel;
      for(int y = 0;y < dst->h;y++) {
        for(int x = 0;x < dst->w;x++) {
          int srcx = x * denominator / numerator;
          int srcy = y * denominator / numerator;
          Uint8 color00[4], color01[4], color10[4], color11[4];
          getpixel(src, srcx, srcy, color00);
          getpixel(src, srcx + 1, srcy, color01);
          getpixel(src, srcx, srcy + 1, color10);
          getpixel(src, srcx + 1, srcy + 1, color11);
          Uint8 color0[4], color1[4], color[4];
          int remx = x * denominator % numerator;
          merge(color0, color00, color01, remx, numerator);
          merge(color1, color10, color11, remx, numerator);
          int remy = y * denominator % numerator;
          merge(color, color0, color1, remy, numerator);
          Uint8 *dstpixel = (Uint8 *) dst->pixels + y * dst->pitch + x * bpp;
          Uint32 mapped = SDL_MapRGBA(dst->format, color[0], color[1], color[2], color[3]);
          switch(bpp) {
            case 1:
              *dstpixel = mapped;
              break;
            case 2:
              *(Uint16 *)dstpixel = mapped;
              break;
            case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
              dstpixel[0] = (mapped >> 16) & 0xff;
              dstpixel[1] = (mapped >> 8) & 0xff;
              dstpixel[2] = (mapped >> 0) & 0xff;
#else
              dstpixel[0] = (mapped >> 0) & 0xff;
              dstpixel[1] = (mapped >> 8) & 0xff;
              dstpixel[2] = (mapped >> 16) & 0xff;
#endif
              break;
            case 4:
              *(Uint32 *)dstpixel = mapped;
              break;
          }
        }
      }
      return dst;
    }
  }
#endif

  // FIXME: Horizontal and vertical line problem
#ifdef BRESENHAM
  void accumulate(SDL_Surface *src, int srcx, int srcy, int color[4], int weight)
  {
    if(srcx < 0 || srcy < 0 || weight == 0) {
      return;
    }
    int bpp = src->format->BytesPerPixel;
    Uint8 *srcpixel = (Uint8 *) src->pixels + srcy * src->pitch + srcx * bpp;
    Uint32 mapped = 0;
    switch(bpp) {
      case 1:
        mapped = *srcpixel;
        break;
      case 2:
        mapped = *(Uint16 *)srcpixel;
        break;
      case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        mapped |= srcpixel[0] << 16;
        mapped |= srcpixel[1] << 8;
        mapped |= srcpixel[2] << 0;
#else
        mapped |= srcpixel[0] << 0;
        mapped |= srcpixel[1] << 8;
        mapped |= srcpixel[2] << 16;
#endif
        break;
      case 4:
        mapped = *(Uint32 *)srcpixel;
        break;
    }
    Uint8 red, green, blue, alpha;
    SDL_GetRGBA(mapped, src->format, &red, &green, &blue, &alpha);
    color[0] += red * weight;
    color[1] += green * weight;
    color[2] += blue * weight;
    color[3] += alpha * weight;
  }

  void accumulate_line(SDL_Surface *src, int srcy, int line[][4], int linesize, int weight, int numerator, int denominator)
  {
    int intpart = denominator / numerator;
    int fractpart = denominator % numerator;
    for(int x = 0, xe = 0, srcx = 0;x < linesize;x++) {
      accumulate(src, srcx, srcy, line[x], (numerator - xe) * weight);
      srcx++;
      for(int i = 0;i < intpart - 1;i++) {
        accumulate(src, srcx, srcy, line[x], numerator * weight);
        srcx++;
      }
      xe += fractpart;
      if(xe >= numerator) {
        xe -= numerator;
        srcx++;
      }
      accumulate(src, srcx, srcy, line[x], xe * weight);
    }
  }

  SDL_Surface *scale(SDL_Surface *src, int numerator, int denominator)
  {
    if(numerator == denominator)
    {
      src->refcount++;
      return src;
    }
    else
    {
      SDL_Surface *dst = SDL_CreateRGBSurface(src->flags, src->w * numerator / denominator, src->h * numerator / denominator, src->format->BitsPerPixel, src->format->Rmask,  src->format->Gmask, src->format->Bmask, src->format->Amask);
      int bpp = dst->format->BytesPerPixel;
      int intpart = denominator / numerator;
      int fractpart = denominator % numerator;
      for(int y = 0, ye = 0, srcy = 0;y < dst->h;y++) {
        int line[dst->w][4];
        memset(line, 0, sizeof(int) * dst->w * 4);
        accumulate_line(src, srcy, line, dst->w, numerator - ye, numerator, denominator);
        srcy++;
        for(int i = 0;i < intpart - 1;i++) {
          accumulate_line(src, srcy, line, dst->w, numerator, numerator, denominator);
          srcy++;
        }
        ye += fractpart;
        if(ye >= numerator) {
          ye -= numerator;
          srcy++;
        }
        accumulate_line(src, srcy, line, dst->w, ye, numerator, denominator);
        for(int x = 0;x < dst->w;x++) {
          Uint8 *dstpixel = (Uint8 *) dst->pixels + y * dst->pitch + x * bpp;
          Uint32 mapped = SDL_MapRGBA(dst->format, line[x][0] / (denominator * denominator), line[x][1] / (denominator * denominator), line[x][2] / (denominator * denominator), line[x][3] / (denominator * denominator));
          switch(bpp) {
            case 1:
              *dstpixel = mapped;
              break;
            case 2:
              *(Uint16 *)dstpixel = mapped;
              break;
            case 3:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
              dstpixel[0] = (mapped >> 16) & 0xff;
              dstpixel[1] = (mapped >> 8) & 0xff;
              dstpixel[2] = (mapped >> 0) & 0xff;
#else
              dstpixel[0] = (mapped >> 0) & 0xff;
              dstpixel[1] = (mapped >> 8) & 0xff;
              dstpixel[2] = (mapped >> 16) & 0xff;
#endif
              break;
            case 4:
              *(Uint32 *)dstpixel = mapped;
              break;
          }
        }
      }
      return dst;
    }
  }
#endif

  SDL_Surface *horz_flip(SDL_Surface *src)
  {
    SDL_Surface *dst = SDL_CreateRGBSurface(src->flags, src->w, src->h, src->format->BitsPerPixel, src->format->Rmask,  src->format->Gmask, src->format->Bmask, src->format->Amask);
    int bpp = dst->format->BytesPerPixel;
    for(int y = 0;y < dst->h;y++) {
      for(int x = 0;x < dst->w;x++) {
        Uint8 *srcpixel = (Uint8 *) src->pixels + y * src->pitch + x * bpp;
        Uint8 *dstpixel = (Uint8 *) dst->pixels + y * dst->pitch + (dst->w - x - 1) * bpp;
        switch(bpp) {
          case 4:
            dstpixel[3] = srcpixel[3];
          case 3:
            dstpixel[2] = srcpixel[2];
          case 2:
            dstpixel[1] = srcpixel[1];
          case 1:
            dstpixel[0] = srcpixel[0];
        }
      }
    }
    return dst;
  }

  SDL_Surface *vert_flip(SDL_Surface *src)
  {
    SDL_Surface *dst = SDL_CreateRGBSurface(src->flags, src->w, src->h, src->format->BitsPerPixel, src->format->Rmask,  src->format->Gmask, src->format->Bmask, src->format->Amask);
    int bpp = dst->format->BytesPerPixel;
    for(int y = 0;y < dst->h;y++) {
      for(int x = 0;x < dst->w;x++) {
        Uint8 *srcpixel = (Uint8 *) src->pixels + y * src->pitch + x * bpp;
        Uint8 *dstpixel = (Uint8 *) dst->pixels + (dst->h - y - 1) * dst->pitch + x * bpp;
        switch(bpp) {
          case 4:
            dstpixel[3] = srcpixel[3];
          case 3:
            dstpixel[2] = srcpixel[2];
          case 2:
            dstpixel[1] = srcpixel[1];
          case 1:
            dstpixel[0] = srcpixel[0];
        }
      }
    }
    return dst;
  }
}

void
Surface::draw_part(float src_x, float src_y, float dst_x, float dst_y,
#ifdef HAVE_OPENGL
                   float width, float height, float alpha,
#else
                   float width, float height, float,
#endif
                   DrawingEffect effect) const
{
#ifdef HAVE_OPENGL
  if(use_opengl)
  {
    float uv_width = surface.opengl.uv_right - surface.opengl.uv_left;
    float uv_height = surface.opengl.uv_bottom - surface.opengl.uv_top;

    float uv_left = surface.opengl.uv_left + (uv_width * src_x) / surface.opengl.width;
    float uv_top = surface.opengl.uv_top + (uv_height * src_y) / surface.opengl.height;
    float uv_right = surface.opengl.uv_left + (uv_width * (src_x + width)) / surface.opengl.width;
    float uv_bottom = surface.opengl.uv_top + (uv_height * (src_y + height)) / surface.opengl.height;

    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBindTexture(GL_TEXTURE_2D, texture->get_handle());
    intern_draw(dst_x, dst_y,
                dst_x + width, dst_y + height,
                uv_left, uv_top, uv_right, uv_bottom, effect);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  }
  else
#endif
  {
    //FIXME: support parameter "alpha"
 
    // get and check SDL_Surface
    if (texture->get_surface() == 0) {
      std::cerr << "Warning: Tried to draw NULL surface, skipped draw" << std::endl;
      return;
    }

    if (surface.sdl.flipx) effect = HORIZONTAL_FLIP;

    float xfactor = (float) config->screenwidth / SCREEN_WIDTH;
    float yfactor = (float) config->screenheight / SCREEN_HEIGHT;
    int numerator;
    int denominator;
    if(xfactor < yfactor)
    {
      numerator = config->screenwidth;
      denominator = SCREEN_WIDTH;
    }
    else
    {
      numerator = config->screenheight;
      denominator = SCREEN_HEIGHT;
    }

    if(transforms[effect] == 0) {
      if(transforms[NO_EFFECT] == 0) {
        transforms[NO_EFFECT] = scale(texture->get_surface(), numerator, denominator);
      }
      switch(effect) {
        case NO_EFFECT:
          break;
        case HORIZONTAL_FLIP:
          transforms[HORIZONTAL_FLIP] = horz_flip(transforms[NO_EFFECT]);
          break;
        case VERTICAL_FLIP:
          transforms[VERTICAL_FLIP] = vert_flip(transforms[NO_EFFECT]);
          break;
        default:
          std::cerr << "Warning: No known transformation applies to surface, skipped draw" << std::endl;
          return;
      }
    }

    int ox = surface.sdl.offsetx;
    if (effect == HORIZONTAL_FLIP) ox = static_cast<int>(texture->get_surface()->w) - (ox+static_cast<int>(width));
    int oy = surface.sdl.offsety;
    if (effect == VERTICAL_FLIP) oy = static_cast<int>(texture->get_surface()->h) - (oy+static_cast<int>(height));
    // draw surface to screen
    SDL_Surface* screen = SDL_GetVideoSurface();

    SDL_Rect srcRect;
    srcRect.x = static_cast<int>((ox + src_x) * numerator / denominator);
    srcRect.y = static_cast<int>((oy + src_y) * numerator / denominator);
    srcRect.w = static_cast<int>(width * numerator / denominator);
    srcRect.h = static_cast<int>(height * numerator / denominator);

    SDL_Rect dstRect;
    dstRect.x = static_cast<int>(dst_x * numerator / denominator);
    dstRect.y = static_cast<int>(dst_y * numerator / denominator);

    SDL_BlitSurface(transforms[effect], &srcRect, screen, &dstRect);
  }
}
