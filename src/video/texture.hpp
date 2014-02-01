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

#ifndef __TEXTURE_HPP__
#define __TEXTURE_HPP__

#include <config.h>

#include <assert.h>

#include <SDL.h>

#include "gameconfig.hpp"
#include "glutil.hpp"

/**
 * This class is a wrapper around a texture handle. It stores the texture width
 * and height and provides convenience functions for uploading SDL_Surfaces
 * into the texture
 */
class Texture
{
protected:
#ifdef HAVE_OPENGL
bool use_opengl;
union
{
struct
{
  GLuint handle;
  unsigned int width;
  unsigned int height;
} opengl;
#else
struct
{
#endif
  SDL_Surface *sdl;
} surface;

public:
  Texture(unsigned int width, unsigned int height, GLenum glformat);
  Texture(SDL_Surface* sdlsurface, GLenum glformat);
  virtual ~Texture();

#ifdef HAVE_OPENGL
  const GLuint &get_handle() const {
    assert(use_opengl);
    return surface.opengl.handle;
  }

  void set_handle(GLuint handle) {
    assert(use_opengl);
    surface.opengl.handle = handle;
  }
#endif

  SDL_Surface *get_surface() const
  {
#ifdef HAVE_OPENGL
    assert(!use_opengl);
#endif
    return surface.sdl;
  }

  void set_surface(SDL_Surface *sdlsurface)
  {
#ifdef HAVE_OPENGL
    assert(!use_opengl);
#endif
    surface.sdl = sdlsurface;
  }

  unsigned int get_width() const
  {
#ifdef HAVE_OPENGL
    if(use_opengl)
    {
      return surface.opengl.width;
    }
    else
#endif
    {
      return surface.sdl->w;
    }
  }

  unsigned int get_height() const
  {
#ifdef HAVE_OPENGL
    if(use_opengl)
    {
      return surface.opengl.height;
    }
    else
#endif
    {
      return surface.sdl->h;
    }
  }

private:
  void set_texture_params();
};

#endif
