//  src/audio/sdl_sound_source.hpp
//
//  SuperTux
//  Copyright (C) 2025 DeltaResero
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

#ifndef __SDL_SOUND_SOURCE_HPP__
#define __SDL_SOUND_SOURCE_HPP__

#include <config.h>

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>
#include "sound_source.hpp"

class SDLSoundSource : public SoundSource
{
public:
  SDLSoundSource(Mix_Chunk* chunk);
  virtual ~SDLSoundSource();

  virtual void play();
  virtual void stop();
  virtual bool playing();

  virtual void set_looping(bool looping);
  virtual void set_gain(float gain);
  virtual void set_pitch(float pitch);
  virtual void set_position(const Vector& position);
  virtual void set_velocity(const Vector& velocity);
  virtual void set_reference_distance(float distance);
  virtual void set_rollof_factor(float factor);

private:
  Mix_Chunk* chunk;
  int channel;
  bool looping;
  float gain;
};

#endif

#endif

// EOF
