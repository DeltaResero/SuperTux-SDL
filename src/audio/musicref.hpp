//  src/audio/musicref.hpp
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

#ifndef __SUPERTUX_MUSICREF_HPP__
#define __SUPERTUX_MUSICREF_HPP__

#ifdef USE_SDL_MIXER

#include "sound_manager.hpp"

/** This class holds a reference to a music file and maintains a correct
 * refcount for that file.
 */
class MusicRef
{
public:
  MusicRef();
  MusicRef(const MusicRef& other);
  ~MusicRef();

  MusicRef& operator= (const MusicRef& other);

private:
  friend class SoundManager;
  // Note: SoundManager::MusicResource must be defined in sound_manager.h
  // when USE_SDL_MIXER is active.
  MusicRef(SoundManager::MusicResource* music);

  SoundManager::MusicResource* music;
};

#endif // USE_SDL_MIXER

#endif /*SUPERTUX_MUSICREF_H*/

// EOF
