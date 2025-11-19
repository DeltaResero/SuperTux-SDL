//  src/audio/sound_manager.hpp
//
//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef __SOUND_MANAGER_H__
#define __SOUND_MANAGER_H__

#include <string>
#include <vector>
#include <map>

#ifdef HAVE_OPENAL
#include <AL/alc.h>
#include <AL/al.h>
#endif

#ifdef USE_SDL_MIXER
#include <SDL_mixer.h>
#endif

#include "math/vector.hpp"

class SoundFile;
class SoundSource;
class StreamSoundSource;
class OpenALSoundSource;

class SoundManager
{
public:
  SoundManager();
  virtual ~SoundManager();

  void enable_sound(bool sound_enabled);
  /**
   * Creates a new sound source object which plays the specified soundfile.
   * You are responsible for deleting the sound source later (this will stop the
   * sound).
   * This function might throw exceptions. It returns 0 if no audio device is
   * available.
   */
  SoundSource* create_sound_source(const std::string& filename);
  /**
   * Convenience function to simply play a sound at a given position.
   */
  void play(const std::string& name, const Vector& pos = Vector(-1, -1));
  /**
   * Adds the source to the list of managed sources (= the source gets deleted
   * when it finished playing)
   */
  void manage_source(SoundSource* source);
  /// preloads a sound, so that you don't get a lag later when playing it
  void preload(const std::string& name);

  void set_listener_position(const Vector& position);
  void set_listener_velocity(const Vector& velocity);

  void enable_music(bool music_enabled);
  void play_music(const std::string& filename, bool fade = false);
  void stop_music(float fadetime = 0);

  bool is_music_enabled() { return music_enabled; }
  bool is_sound_enabled() { return sound_enabled; }

  bool is_audio_enabled();

  void update();

  /*
   * Tell soundmanager to call update() for stream_sound_source.
   */
  void register_for_update( StreamSoundSource* sss );
  /*
   * Unsubscribe from updates for stream_sound_source.
   */
  void remove_from_update( StreamSoundSource* sss );

#ifdef USE_SDL_MIXER
  // Needed by MusicRef
  struct MusicResource {
      ~MusicResource();
      SoundManager* manager;
      Mix_Music* music;
      int refcount;
  };
  void free_music(MusicResource* music);
#endif

private:
  friend class OpenALSoundSource;
  friend class StreamSoundSource;
#ifdef USE_SDL_MIXER
  friend class MusicRef;
#endif

#ifdef HAVE_OPENAL
  static ALuint load_file_into_buffer(SoundFile* file);
  static ALenum get_sample_format(SoundFile* file);

  void print_openal_version();
  void check_alc_error(const char* message);
  static void check_al_error(const char* message);

  ALCdevice* device;
  ALCcontext* context;

  typedef std::map<std::string, ALuint> SoundBuffers;
  SoundBuffers buffers;

  StreamSoundSource* music_source;
#endif

#ifdef USE_SDL_MIXER
  typedef std::map<std::string, Mix_Chunk*> SoundChunks;
  SoundChunks sound_chunks;

  typedef std::map<std::string, MusicResource> Musics;
  Musics musics;

  MusicResource* current_music_resource;
#endif

  bool sound_enabled;
  bool music_enabled;
  std::string current_music;

  // Changed from OpenALSoundSource* to SoundSource* to support polymorphism
  typedef std::vector<SoundSource*> SoundSources;
  SoundSources sources;

  typedef std::vector<StreamSoundSource*> StreamSoundSources;
  StreamSoundSources update_list;
};

extern SoundManager* sound_manager;

#endif

// EOF
