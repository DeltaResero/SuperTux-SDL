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

#include <fstream>
#include <sstream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdexcept>

#include <SDL.h>

#include "game_session.hpp"
#include "log.hpp"
#include "console.hpp"
#include "worldmap/worldmap.hpp"
#include "mainloop.hpp"
#include "audio/sound_manager.hpp"
#include "gui/menu.hpp"
#include "sector.hpp"
#include "level.hpp"
#include "tile.hpp"
#include "player_status.hpp"
#include "object/particlesystem.hpp"
#include "object/background.hpp"
#include "object/gradient.hpp"
#include "object/tilemap.hpp"
#include "object/camera.hpp"
#include "object/player.hpp"
#include "object/level_time.hpp"
#include "lisp/lisp.hpp"
#include "lisp/parser.hpp"
#include "resources.hpp"
#include "statistics.hpp"
#include "timer.hpp"
#include "options_menu.hpp"
#include "textscroller.hpp"
#include "control/codecontroller.hpp"
#include "control/joystickkeyboardcontroller.hpp"
#include "main.hpp"
#include "file_system.hpp"
#include "gameconfig.hpp"
#include "gettext.hpp"
#include "console.hpp"
#include "flip_level_transformer.hpp"
#include "trigger/secretarea_trigger.hpp"
#include "trigger/sequence_trigger.hpp"
#include "random_generator.hpp"
#include "scripting/squirrel_util.hpp"
#include "object/endsequence_walkright.hpp"
#include "object/endsequence_walkleft.hpp"
#include "object/endsequence_fireworks.hpp"
#include "direction.hpp"
#include "scripting/time_scheduler.hpp"

// the engine will be run with a logical framerate of 64fps.
// We chose 64fps here because it is a power of 2, so 1/64 gives an "even"
// binary fraction...
static const float LOGICAL_FPS = 64.0;

enum GameMenuIDs {
  MNID_CONTINUE,
  MNID_ABORTLEVEL
};

GameSession* GameSession::current_ = NULL;

GameSession::GameSession(const std::string& levelfile_, Statistics* statistics)
  : level(0), currentsector(0),
    end_sequence(0),
    levelfile(levelfile_), best_level_statistics(statistics),
    capture_demo_stream(0), playback_demo_stream(0), demo_controller(0),
    play_time(0), edit_mode(false)
{
  current_ = this;
  currentsector = NULL;

  game_pause = false;
  speed_before_pause = main_loop->get_speed();

  statistics_backdrop.reset(new Surface("images/engine/menu/score-backdrop.png"));

  restart_level();

  game_menu.reset(new Menu());
  game_menu->add_label(_("Pause"));
  game_menu->add_hl();
  game_menu->add_entry(MNID_CONTINUE, _("Continue"));
  game_menu->add_submenu(_("Options"), get_options_menu());
  game_menu->add_hl();
  game_menu->add_entry(MNID_ABORTLEVEL, _("Abort Level"));
}

void
GameSession::restart_level()
{

  if (edit_mode) {
    force_ghost_mode();
    return;
  }

  game_pause   = false;
  end_sequence = 0;

  main_controller->reset();

  currentsector = 0;

  level.reset(new Level);
  level->load(levelfile);
  level->stats.total_coins = level->get_total_coins();
  level->stats.total_badguys = level->get_total_badguys();
  level->stats.total_secrets = level->get_total_count<SecretAreaTrigger>();
  level->stats.reset();
  if(reset_sector != "")level->stats.declare_invalid();

  if(reset_sector != "") {
    currentsector = level->get_sector(reset_sector);
    if(!currentsector) {
      std::stringstream msg;
      msg << "Couldn't find sector '" << reset_sector << "' for resetting tux.";
      throw std::runtime_error(msg.str());
    }
    currentsector->activate(reset_pos);
  } else {
    currentsector = level->get_sector("main");
    if(!currentsector)
      throw std::runtime_error("Couldn't find main sector");
    currentsector->activate("main");
  }

  //levelintro();

  sound_manager->stop_music();
  currentsector->play_music(LEVEL_MUSIC);

  if(capture_file != "") {
    int newSeed=0;               // next run uses a new seed
    while (newSeed == 0)            // which is the next non-zero random num.
        newSeed = systemRandom.rand();
    config->random_seed = systemRandom.srand(newSeed);
    log_info << "Next run uses random seed " <<config->random_seed <<std::endl;
    record_demo(capture_file);
  }
}

GameSession::~GameSession()
{
  delete capture_demo_stream;
  delete playback_demo_stream;
  delete demo_controller;
  free_options_menu();

  current_ = NULL;
}

void
GameSession::record_demo(const std::string& filename)
{
  delete capture_demo_stream;

  capture_demo_stream = new std::ofstream(filename.c_str());
  if(!capture_demo_stream->good()) {
    std::stringstream msg;
    msg << "Couldn't open demo file '" << filename << "' for writing.";
    throw std::runtime_error(msg.str());
  }
  capture_file = filename;

  char buf[30];                            // save the seed in the demo file
  snprintf(buf, sizeof(buf), "random_seed=%10d", config->random_seed);
  for (int i=0; i==0 || buf[i-1]; i++)
    capture_demo_stream->put(buf[i]);
}

int
GameSession::get_demo_random_seed(const std::string& filename)
{
  std::istream* test_stream = new std::ifstream(filename.c_str());
  if(test_stream->good()) {
    char buf[30];                     // recall the seed from the demo file
    int seed;
    for (int i=0; i<30 && (i==0 || buf[i-1]); i++)
      test_stream->get(buf[i]);
    if (sscanf(buf, "random_seed=%10d", &seed) == 1) {
      log_info << "Random seed " << seed << " from demo file" << std::endl;
         return seed;
    }
    else
      log_info << "Demo file contains no random number" << std::endl;
  }
  return 0;
}

void
GameSession::play_demo(const std::string& filename)
{
  delete playback_demo_stream;
  delete demo_controller;

  playback_demo_stream = new std::ifstream(filename.c_str());
  if(!playback_demo_stream->good()) {
    std::stringstream msg;
    msg << "Couldn't open demo file '" << filename << "' for reading.";
    throw std::runtime_error(msg.str());
  }

  Player& tux = *currentsector->player;
  demo_controller = new CodeController();
  tux.set_controller(demo_controller);

  // skip over random seed, if it exists in the file
  char buf[30];                            // ascii decimal seed
  int seed;
  for (int i=0; i<30 && (i==0 || buf[i-1]); i++)
    playback_demo_stream->get(buf[i]);
  if (sscanf(buf, "random_seed=%010d", &seed) != 1)
    playback_demo_stream->seekg(0);     // old style w/o seed, restart at beg
}

void
GameSession::levelintro()
{
  sound_manager->stop_music();

  DrawingContext context;
  for(Sector::GameObjects::iterator i = currentsector->gameobjects.begin();
      i != currentsector->gameobjects.end(); ++i) {
    Background* background = dynamic_cast<Background*> (*i);
    if(background) {
      background->draw(context);
    }
    Gradient* gradient = dynamic_cast<Gradient*> (*i);
    if(gradient) {
      gradient->draw(context);
    }
  }

//  context.draw_text(gold_text, level->get_name(), Vector(SCREEN_WIDTH/2, 160),
//      ALIGN_CENTER, LAYER_FOREGROUND1);
  context.draw_center_text(gold_text, level->get_name(), Vector(0, 160),
      LAYER_FOREGROUND1);

  std::stringstream ss_coins;
  ss_coins << _("Coins") << ": " << player_status->coins;
  context.draw_text(white_text, ss_coins.str(), Vector(SCREEN_WIDTH/2, 210),
      ALIGN_CENTER, LAYER_FOREGROUND1);

  if((level->get_author().size()) && (level->get_author() != "SuperTux Team"))
    context.draw_text(white_small_text,
      std::string(_("contributed by ")) + level->get_author(),
      Vector(SCREEN_WIDTH/2, 350), ALIGN_CENTER, LAYER_FOREGROUND1);

  if(best_level_statistics != NULL)
    best_level_statistics->draw_message_info(context, _("Best Level Statistics"));

  wait_for_event(1.0, 3.0);
}

void
GameSession::on_escape_press()
{
  if(currentsector->player->is_dying() || end_sequence)
  {
    // Let the timers run out, we fast-forward them to force past a sequence
    if (end_sequence)
      end_sequence->stop();

    currentsector->player->dying_timer.start(FLT_EPSILON);
    return;   // don't let the player open the menu, when he is dying
  }

  if(level->on_menukey_script != "") {
    std::istringstream in(level->on_menukey_script);
    run_script(in, "OnMenuKeyScript");
  } else {
    toggle_pause();
  }
}

void
GameSession::toggle_pause()
{
  if(!game_pause) {
    speed_before_pause = main_loop->get_speed();
    main_loop->set_speed(0);
    Menu::set_current(game_menu.get());
    game_menu->set_active_item(MNID_CONTINUE);
    game_pause = true;
  } else {
    main_loop->set_speed(speed_before_pause);
    Menu::set_current(NULL);
    game_pause = false;
  }
}

void
GameSession::set_editmode(bool edit_mode)
{
  if (this->edit_mode == edit_mode) return;
  this->edit_mode = edit_mode;

  currentsector->get_players()[0]->set_edit_mode(edit_mode);

  if (edit_mode) {

    // entering edit mode

  } else {

    // leaving edit mode
    restart_level();

  }
}

void
GameSession::force_ghost_mode()
{
  currentsector->get_players()[0]->set_ghost_mode(true);
}

HSQUIRRELVM
GameSession::run_script(std::istream& in, const std::string& sourcename)
{
  using namespace Scripting;

  // garbage collect thread list
  for(ScriptList::iterator i = scripts.begin();
      i != scripts.end(); ) {
    HSQOBJECT& object = *i;
    HSQUIRRELVM vm = object_to_vm(object);

    if(sq_getvmstate(vm) != SQ_VMSTATE_SUSPENDED) {
      sq_release(global_vm, &object);
      i = scripts.erase(i);
      continue;
    }

    ++i;
  }

  HSQOBJECT object = create_thread(global_vm);
  scripts.push_back(object);

  HSQUIRRELVM vm = object_to_vm(object);

  compile_and_run(vm, in, sourcename);

  return vm;
}

void
GameSession::process_events()
{
  // end of pause mode?
  if(!Menu::current() && game_pause) {
    game_pause = false;
  }

  // playback a demo?
  if(playback_demo_stream != 0) {
    demo_controller->update();
    char left = false;
    char right = false;
    char up = false;
    char down = false;
    char jump = false;
    char action = false;
    playback_demo_stream->get(left);
    playback_demo_stream->get(right);
    playback_demo_stream->get(up);
    playback_demo_stream->get(down);
    playback_demo_stream->get(jump);
    playback_demo_stream->get(action);
    demo_controller->press(Controller::LEFT, left);
    demo_controller->press(Controller::RIGHT, right);
    demo_controller->press(Controller::UP, up);
    demo_controller->press(Controller::DOWN, down);
    demo_controller->press(Controller::JUMP, jump);
    demo_controller->press(Controller::ACTION, action);
  }

  // save input for demo?
  if(capture_demo_stream != 0) {
    capture_demo_stream ->put(main_controller->hold(Controller::LEFT));
    capture_demo_stream ->put(main_controller->hold(Controller::RIGHT));
    capture_demo_stream ->put(main_controller->hold(Controller::UP));
    capture_demo_stream ->put(main_controller->hold(Controller::DOWN));
    capture_demo_stream ->put(main_controller->hold(Controller::JUMP));
    capture_demo_stream ->put(main_controller->hold(Controller::ACTION));
  }
}

void
GameSession::check_end_conditions()
{
  Player* tux = currentsector->player;

  /* End of level? */
  if(end_sequence && end_sequence->is_done()) {
    finish(true);
  } else if (!end_sequence && tux->is_dead()) {
    restart_level();
  }
}

void
GameSession::draw(DrawingContext& context)
{
  currentsector->draw(context);
  drawstatus(context);

  if(game_pause)
    draw_pause(context);
}

void
GameSession::draw_pause(DrawingContext& context)
{
  context.draw_filled_rect(
      Vector(0,0), Vector(SCREEN_WIDTH, SCREEN_HEIGHT),
      Color(.2f, .2f, .2f, .5f), LAYER_FOREGROUND1);
}

void
GameSession::process_menu()
{
  Menu* menu = Menu::current();
  if(menu) {
    menu->update();

    if(menu == game_menu.get()) {
      switch (game_menu->check()) {
        case MNID_CONTINUE:
          Menu::set_current(0);
          toggle_pause();
          break;
        case MNID_ABORTLEVEL:
          Menu::set_current(0);
          main_loop->exit_screen();
          break;
      }
    }
  }
}

void
GameSession::setup()
{
  Menu::set_current(NULL);
  current_ = this;

  if(currentsector != Sector::current()) {
	currentsector->activate(currentsector->player->get_pos());
  }
  currentsector->play_music(LEVEL_MUSIC);

  // Eat unneeded events
  SDL_Event event;
  while(SDL_PollEvent(&event))
  {}
}

void
GameSession::update(float elapsed_time)
{
  // handle controller
  if(main_controller->pressed(Controller::PAUSE_MENU))
    on_escape_press();

  process_events();
  process_menu();

  check_end_conditions();

  // respawning in new sector?
  if(newsector != "" && newspawnpoint != "") {
    Sector* sector = level->get_sector(newsector);
    if(sector == 0) {
      log_warning << "Sector '" << newsector << "' not found" << std::endl;
    }
    sector->activate(newspawnpoint);
    sector->play_music(LEVEL_MUSIC);
    currentsector = sector;
    newsector = "";
    newspawnpoint = "";
  }

  // Update the world state and all objects in the world
  if(!game_pause) {
    // Update the world
    if (!end_sequence) {
      play_time += elapsed_time; //TODO: make sure we don't count cutscene time
      level->stats.time = play_time;
      currentsector->update(elapsed_time);
    } else {
      if (!end_sequence->is_tux_stopped()) {
        currentsector->update(elapsed_time);
      } else {
        end_sequence->update(elapsed_time);
      }
    }
  }

  // update sounds
  sound_manager->set_listener_position(currentsector->player->get_pos());

  /* Handle music: */
  if (end_sequence)
    return;

  if(currentsector->player->invincible_timer.started()) {
    if(currentsector->player->invincible_timer.get_timeleft() <=
       TUX_INVINCIBLE_TIME_WARNING) {
      currentsector->play_music(HERRING_WARNING_MUSIC);
    } else {
      currentsector->play_music(HERRING_MUSIC);
    }
  } else if(currentsector->get_music_type() != LEVEL_MUSIC) {
    currentsector->play_music(LEVEL_MUSIC);
  }
}

void
GameSession::finish(bool win)
{
  using namespace WorldMapNS;

  if (edit_mode) {
    force_ghost_mode();
    return;
  }

  if(win) {
    if(WorldMap::current())
      WorldMap::current()->finished_level(level.get());
  }

  main_loop->exit_screen();
}

void
GameSession::respawn(const std::string& sector, const std::string& spawnpoint)
{
  newsector = sector;
  newspawnpoint = spawnpoint;
}

void
GameSession::set_reset_point(const std::string& sector, const Vector& pos)
{
  reset_sector = sector;
  reset_pos = pos;
}

std::string
GameSession::get_working_directory()
{
  return FileSystem::dirname(levelfile);
}

void
GameSession::start_sequence(const std::string& sequencename)
{
  // do not play sequences when in edit mode
  if (edit_mode) {
    force_ghost_mode();
    return;
  }

  // handle special "stoptux" sequence
  if (sequencename == "stoptux") {
    if (!end_sequence) {
      log_warning << "Final target reached without an active end sequence" << std::endl;
      this->start_sequence("endsequence");
    }
    if (end_sequence) end_sequence->stop_tux();
    return;
  }

  // abort if a sequence is already playing
  if (end_sequence)
	  return;

  if (sequencename == "endsequence") {
    if (currentsector->get_players()[0]->physic.get_velocity_x() < 0) {
      end_sequence = new EndSequenceWalkLeft();
    } else {
      end_sequence = new EndSequenceWalkRight();
    }
  } else if (sequencename == "fireworks") {
    end_sequence = new EndSequenceFireworks();
  } else {
    log_warning << "Unknown sequence '" << sequencename << "'. Ignoring." << std::endl;
    return;
  }

  /* slow down the game for end-sequence */
  main_loop->set_speed(0.5f);

  currentsector->add_object(end_sequence);
  end_sequence->start();

  sound_manager->play_music("music/leveldone.ogg", false);
  currentsector->player->invincible_timer.start(10000.0f);

  // Stop all clocks.
  for(std::vector<GameObject*>::iterator i = currentsector->gameobjects.begin();
		  i != currentsector->gameobjects.end(); ++i)
  {
    GameObject* obj = *i;

    LevelTime* lt = dynamic_cast<LevelTime*> (obj);
    if(lt)
      lt->stop();
  }
}

/* (Status): */
void
GameSession::drawstatus(DrawingContext& context)
{
  player_status->draw(context);

  // draw level stats while end_sequence is running
  if (end_sequence) {
    level->stats.draw_endseq_panel(context, best_level_statistics, statistics_backdrop.get());
  }
}
