//  src/audio/sound_manager.cpp
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
#include <config.h>

#include "sound_manager.hpp"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <memory>
#include <assert.h>
#include <SDL.h>

#include "sound_file.hpp"
#include "sound_source.hpp"
#include "dummy_sound_source.hpp"
#include "log.hpp"
#include "timer.hpp"

#ifdef HAVE_OPENAL
#include "openal_sound_source.hpp"
#include "stream_sound_source.hpp"
#endif

#ifdef USE_SDL_MIXER
#include "sdl_sound_source.hpp"
#include "physfs/physfs_sdl.hpp"
#include <physfs.h>
#endif

SoundManager* sound_manager = 0;

SoundManager::SoundManager()
  : sound_enabled(false), music_enabled(false)
{
#ifdef HAVE_OPENAL
  device = 0;
  context = 0;
  music_source = 0;
  try {
    device = alcOpenDevice(0);
    if (device == NULL) {
      throw std::runtime_error("Couldn't open audio device.");
    }

    int attributes[] = { 0 };
    context = alcCreateContext(device, attributes);
    check_alc_error("Couldn't create audio context: ");
    alcMakeContextCurrent(context);
    check_alc_error("Couldn't select audio context: ");

    check_al_error("Audio error after init: ");
    sound_enabled = true;
    music_enabled = true;
  } catch(std::exception& e) {
    if(context != NULL)
      alcDestroyContext(context);
    context = NULL;
    if(device != NULL)
      alcCloseDevice(device);
    device = NULL;
    log_warning << "Couldn't initialize audio device: " << e.what() << std::endl;
    print_openal_version();
  }
#endif

#ifdef USE_SDL_MIXER
  current_music_resource = 0;
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    log_warning << "Couldn't initialize SDL Audio: " << Mix_GetError() << std::endl;
    sound_enabled = false;
  } else {
    Mix_AllocateChannels(32);
    sound_enabled = true;
    music_enabled = true;
  }
#endif
}

SoundManager::~SoundManager()
{
  for(SoundSources::iterator i = sources.begin(); i != sources.end(); ++i) {
    delete *i;
  }

#ifdef HAVE_OPENAL
  delete music_source;

  for(SoundBuffers::iterator i = buffers.begin(); i != buffers.end(); ++i) {
    ALuint buffer = i->second;
    alDeleteBuffers(1, &buffer);
  }

  if(context != NULL) {
    alcDestroyContext(context);
  }
  if(device != NULL) {
    alcCloseDevice(device);
  }
#endif

#ifdef USE_SDL_MIXER
  Mix_HaltMusic();
  // Free chunks
  for(SoundChunks::iterator i = sound_chunks.begin(); i != sound_chunks.end(); ++i) {
    Mix_FreeChunk(i->second);
  }
  sound_chunks.clear();
  // Free music
  if(current_music_resource) {
      current_music_resource->refcount--;
      if(current_music_resource->refcount == 0)
          free_music(current_music_resource);
  }
  Mix_CloseAudio();
#endif
}

#ifdef HAVE_OPENAL
ALuint
SoundManager::load_file_into_buffer(SoundFile* file)
{
  ALenum format = get_sample_format(file);
  ALuint buffer;
  alGenBuffers(1, &buffer);
  check_al_error("Couldn't create audio buffer: ");
  char* samples = new char[file->size];
  try {
    file->read(samples, file->size);
    alBufferData(buffer, format, samples,
        static_cast<ALsizei> (file->size),
        static_cast<ALsizei> (file->rate));
    check_al_error("Couldn't fill audio buffer: ");
  } catch(...) {
    delete[] samples;
    throw;
  }
  delete[] samples;

  return buffer;
}
#endif

SoundSource*
SoundManager::create_sound_source(const std::string& filename)
{
  if(!sound_enabled)
    return create_dummy_sound_source();

#ifdef HAVE_OPENAL
  std::auto_ptr<OpenALSoundSource> source;
  try {
    source.reset(new OpenALSoundSource());
  } catch(std::exception& e) {
    log_warning << "Couldn't create audio source: " << e.what() << std::endl;
    return create_dummy_sound_source();
  }

  ALuint buffer;

  // reuse an existing static sound buffer
  SoundBuffers::iterator i = buffers.find(filename);
  if(i != buffers.end()) {
    buffer = i->second;
  } else {
    try {
      // Load sound file
      std::auto_ptr<SoundFile> file (load_sound_file(filename));

      if(file->size < 100000) {
        buffer = load_file_into_buffer(file.get());
        buffers.insert(std::make_pair(filename, buffer));
      } else {
        StreamSoundSource* source = new StreamSoundSource();
        source->set_sound_file(file.release());
        return source;
      }
    } catch(std::exception& e) {
      log_warning << "Couldn't load soundfile '" << filename << "': " << e.what() << std::endl;
      return create_dummy_sound_source();
    }
  }

  alSourcei(source->source, AL_BUFFER, buffer);
  return source.release();
#endif

#ifdef USE_SDL_MIXER
  Mix_Chunk* chunk = 0;
  SoundChunks::iterator i = sound_chunks.find(filename);
  if(i != sound_chunks.end()) {
    chunk = i->second;
  } else {
    // Load new chunk
    chunk = Mix_LoadWAV_RW(get_physfs_SDLRWops(filename), 1);
    if(chunk) {
      sound_chunks.insert(std::make_pair(filename, chunk));
    } else {
      log_warning << "Couldn't load sound '" << filename << "': " << Mix_GetError() << std::endl;
      return create_dummy_sound_source();
    }
  }
  return new SDLSoundSource(chunk);
#endif

  return create_dummy_sound_source();
}

void
SoundManager::preload(const std::string& filename)
{
  if(!sound_enabled)
    return;

#ifdef HAVE_OPENAL
  SoundBuffers::iterator i = buffers.find(filename);
  // already loaded?
  if(i != buffers.end())
    return;

  std::auto_ptr<SoundFile> file (load_sound_file(filename));
  // only keep small files
  if(file->size >= 100000)
    return;

  ALuint buffer = load_file_into_buffer(file.get());
  buffers.insert(std::make_pair(filename, buffer));
#endif

#ifdef USE_SDL_MIXER
  if(sound_chunks.find(filename) == sound_chunks.end()) {
      Mix_Chunk* chunk = Mix_LoadWAV_RW(get_physfs_SDLRWops(filename), 1);
      if(chunk) {
          sound_chunks.insert(std::make_pair(filename, chunk));
      }
  }
#endif
}

void
SoundManager::play(const std::string& filename, const Vector& pos)
{
  if(!sound_enabled)
    return;

  try {
    SoundSource* source = create_sound_source(filename);
    if(source == 0) return;

    if(pos == Vector(-1, -1)) {
      source->set_rollof_factor(0);
    } else {
      source->set_position(pos);
    }
    source->play();
    manage_source(source);
  } catch(std::exception& e) {
    log_warning << "Couldn't play sound " << filename << ": " << e.what() << std::endl;
  }
}

void
SoundManager::manage_source(SoundSource* source)
{
  assert(source != NULL);
  sources.push_back(source);
}

void
SoundManager::register_for_update( StreamSoundSource* sss ){
  if( sss != NULL ){
    update_list.push_back( sss );
  }
}

void
SoundManager::remove_from_update( StreamSoundSource* sss  ){
  if( sss != NULL ){
    StreamSoundSources::iterator i = update_list.begin();
	while( i != update_list.end() ){
      if( *i == sss ){
        i = update_list.erase(i);
      } else {
        i++;
      }
    }
  }
}

void
SoundManager::enable_sound(bool enable)
{
#ifdef HAVE_OPENAL
  if(device == NULL) return;
#endif
  sound_enabled = enable;
}

void
SoundManager::enable_music(bool enable)
{
#ifdef HAVE_OPENAL
  if(device == NULL) return;
#endif
  music_enabled = enable;
  if(music_enabled) {
    play_music(current_music);
  } else {
    stop_music();
  }
}

void
SoundManager::stop_music(float fadetime)
{
#ifdef HAVE_OPENAL
  if(fadetime > 0) {
    if(music_source
        && music_source->get_fade_state() != StreamSoundSource::FadingOff)
      music_source->set_fading(StreamSoundSource::FadingOff, fadetime);
  } else {
    delete music_source;
    music_source = NULL;
  }
#endif

#ifdef USE_SDL_MIXER
  if(fadetime > 0)
    Mix_FadeOutMusic(static_cast<int>(fadetime * 1000));
  else
    Mix_HaltMusic();

  if(current_music_resource) {
      current_music_resource->refcount--;
      if(current_music_resource->refcount == 0)
          free_music(current_music_resource);
      current_music_resource = 0;
  }
#endif
  current_music = "";
}

void
SoundManager::play_music(const std::string& filename, bool fade)
{
  if(filename == current_music)
    return;
  current_music = filename;
  if(!music_enabled)
    return;

  if(filename == "") {
    stop_music();
    return;
  }

#ifdef HAVE_OPENAL
  try {
    std::auto_ptr<StreamSoundSource> newmusic (new StreamSoundSource());
    alSourcef(newmusic->source, AL_ROLLOFF_FACTOR, 0);
    newmusic->set_sound_file(load_sound_file(filename));
    newmusic->set_looping(true);
    if(fade)
      newmusic->set_fading(StreamSoundSource::FadingOn, .5f);
    newmusic->play();

    delete music_source;
    music_source = newmusic.release();
  } catch(std::exception& e) {
    log_warning << "Couldn't play music file '" << filename << "': " << e.what() << std::endl;
  }
#endif

#ifdef USE_SDL_MIXER
  // Release old music
  if(current_music_resource) {
      current_music_resource->refcount--;
      if(current_music_resource->refcount == 0)
          free_music(current_music_resource);
      current_music_resource = 0;
  }

  // Find or load new music
  Musics::iterator i = musics.find(filename);
  if(i != musics.end()) {
      current_music_resource = &(i->second);
  } else {
      Mix_Music* song = Mix_LoadMUS( (std::string(PHYSFS_getRealDir(filename.c_str())) + "/" + filename).c_str() );
      if(song) {
          std::pair<Musics::iterator, bool> result = musics.insert(std::make_pair(filename, MusicResource()));
          current_music_resource = &(result.first->second);
          current_music_resource->manager = this;
          current_music_resource->music = song;
          current_music_resource->refcount = 0;
      } else {
          log_warning << "Couldn't load music '" << filename << "': " << Mix_GetError() << std::endl;
          return;
      }
  }

  if(current_music_resource) {
      current_music_resource->refcount++;
      if(fade)
          Mix_FadeInMusic(current_music_resource->music, -1, 500);
      else
          Mix_PlayMusic(current_music_resource->music, -1);
  }
#endif
}

void
SoundManager::set_listener_position(const Vector& pos)
{
#ifdef HAVE_OPENAL
  static Uint32 lastticks = SDL_GetTicks();

  Uint32 current_ticks = SDL_GetTicks();
  if(current_ticks - lastticks < 300)
    return;
  lastticks = current_ticks;

  alListener3f(AL_POSITION, pos.x, pos.y, 0);
#endif
}

void
SoundManager::set_listener_velocity(const Vector& vel)
{
#ifdef HAVE_OPENAL
  alListener3f(AL_VELOCITY, vel.x, vel.y, 0);
#endif
}

void
SoundManager::update()
{
  static Uint32 lasttime = SDL_GetTicks();
  Uint32 now = SDL_GetTicks();

  if(now - lasttime < 300)
    return;
  lasttime = now;

  // update and check for finished sound sources
  for(SoundSources::iterator i = sources.begin(); i != sources.end(); ) {
    SoundSource* source = *i;

    // OpenAL sources need explicit update, SDL ones don't but it's harmless
#ifdef HAVE_OPENAL
    OpenALSoundSource* al_source = dynamic_cast<OpenALSoundSource*>(source);
    if(al_source) al_source->update();
#endif

    if(!source->playing()) {
      delete source;
      i = sources.erase(i);
    } else {
      ++i;
    }
  }

#ifdef HAVE_OPENAL
  // check streaming sounds
  if(music_source) {
    music_source->update();
  }

  if (context)
  {
    alcProcessContext(context);
    check_alc_error("Error while processing audio context: ");
  }

  //run update() for stream_sound_source
  StreamSoundSources::iterator s = update_list.begin();
  while( s != update_list.end() ){
    (*s)->update();
    s++;
  }
#endif
}

bool
SoundManager::is_audio_enabled() {
#ifdef HAVE_OPENAL
    return device != 0 && context != 0;
#endif
#ifdef USE_SDL_MIXER
    return sound_enabled; // Rough approximation
#endif
    return false;
}

#ifdef USE_SDL_MIXER
SoundManager::MusicResource::~MusicResource()
{
    // Mix_FreeMusic(music); // Handled in free_music
}

void
SoundManager::free_music(MusicResource* res)
{
    if(res->music) {
        Mix_FreeMusic(res->music);
        res->music = 0;
    }
    // Note: We don't erase from map here to avoid iterator invalidation issues
    // if called during iteration, but ideally we should clean up the map.
    // For now, we just free the SDL resource.
}
#endif

#ifdef HAVE_OPENAL
ALenum
SoundManager::get_sample_format(SoundFile* file)
{
  if(file->channels == 2) {
    if(file->bits_per_sample == 16) {
      return AL_FORMAT_STEREO16;
    } else if(file->bits_per_sample == 8) {
      return AL_FORMAT_STEREO8;
    } else {
      throw std::runtime_error("Only 16 and 8 bit samples supported");
    }
  } else if(file->channels == 1) {
    if(file->bits_per_sample == 16) {
      return AL_FORMAT_MONO16;
    } else if(file->bits_per_sample == 8) {
      return AL_FORMAT_MONO8;
    } else {
      throw std::runtime_error("Only 16 and 8 bit samples supported");
    }
  }

  throw std::runtime_error("Only 1 and 2 channel samples supported");
}

void
SoundManager::print_openal_version()
{
  log_info << "OpenAL Vendor: " << alGetString(AL_VENDOR) << std::endl;
  log_info << "OpenAL Version: " << alGetString(AL_VERSION) << std::endl;
  log_info << "OpenAL Renderer: " << alGetString(AL_RENDERER) << std::endl;
  log_info << "OpenAl Extensions: " << alGetString(AL_EXTENSIONS) << std::endl;
}

void
SoundManager::check_alc_error(const char* message)
{
  int err = alcGetError(device);
  if(err != ALC_NO_ERROR) {
    std::stringstream msg;
    msg << message << alcGetString(device, err);
    throw std::runtime_error(msg.str());
  }
}

void
SoundManager::check_al_error(const char* message)
{
  int err = alGetError();
  if(err != AL_NO_ERROR) {
    std::stringstream msg;
    msg << message << alGetString(err);
    throw std::runtime_error(msg.str());
  }
}
#endif

// EOF
