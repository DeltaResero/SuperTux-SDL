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
#ifndef __GLUTIL_HPP__
#define __GLUTIL_HPP__

#include <config.h>

#ifdef HAVE_OPENGL

#include <sstream>
#include <stdexcept>
#include <GL/gl.h>
#include <GL/glext.h>

static inline void check_gl_error(const char* message)
{
#ifdef DEBUG
  GLenum error = glGetError();
  if(error != GL_NO_ERROR) {
    std::ostringstream msg;
    msg << "OpenGLError while '" << message << "': ";
    switch(error) {
      case GL_INVALID_ENUM:
        msg << "INVALID_ENUM: An unacceptable value is specified for an "
               "enumerated argument.";
        break;
      case GL_INVALID_VALUE:
        msg << "INVALID_VALUE: A numeric argument is out of range.";
        break;
      case GL_INVALID_OPERATION:
        msg << "INVALID_OPERATION: The specified operation is not allowed "
               "in the current state.";
        break;
      case GL_STACK_OVERFLOW:
        msg << "STACK_OVERFLOW: This command would cause a stack overflow.";
        break;
      case GL_STACK_UNDERFLOW:
        msg << "STACK_UNDERFLOW: This command would cause a stack underflow.";
        break;
      case GL_OUT_OF_MEMORY:
        msg << "OUT_OF_MEMORY: There is not enough memory left to execute the "
               "command.";
        break;
#ifdef GL_TABLE_TOO_LARGE
      case GL_TABLE_TOO_LARGE:
        msg << "TABLE_TOO_LARGE: table is too large";
        break;
#endif
      default:
        msg << "Unknown error (code " << error << ")";
    }

    throw std::runtime_error(msg.str());
  }
#endif
}

static inline void assert_gl(const char* message)
{
#ifdef DEBUG
  check_gl_error(message);
#else
  (void) message;
#endif
}

#else

#define GLenum int
#define GLint int
#define GL_SRC_ALPHA 0
#define GL_ONE_MINUS_SRC_ALPHA 1
#define GL_RGBA 2
#define GL_ONE 3

#endif

#endif
