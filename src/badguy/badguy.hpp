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

#ifndef __BADGUY_H__
#define __BADGUY_H__

// moved them here to make it less typing when implementing new badguys
#include <math.h>
#include "timer.hpp"
#include "object/moving_sprite.hpp"
#include "physic.hpp"
#include "object/player.hpp"
#include "serializable.hpp"
#include "resources.hpp"
#include "sector.hpp"
#include "direction.hpp"
#include "object_factory.hpp"
#include "lisp/parser.hpp"
#include "lisp/lisp.hpp"
#include "lisp/writer.hpp"
#include "video/drawing_context.hpp"
#include "audio/sound_manager.hpp"
#include "audio/sound_source.hpp"

/**
 * Base class for moving sprites that can hurt the Player.
 */
class BadGuy : public MovingSprite, protected UsesPhysic, public Serializable
{
public:
  BadGuy(const Vector& pos, const std::string& sprite_name, int layer = LAYER_OBJECTS);
  BadGuy(const Vector& pos, Direction direction, const std::string& sprite_name, int layer = LAYER_OBJECTS);
  BadGuy(const lisp::Lisp& reader, const std::string& sprite_name, int layer = LAYER_OBJECTS);

  /** Called when the badguy is drawn. The default implementation simply draws
   * the badguy sprite on screen
   */
  virtual void draw(DrawingContext& context);
  /** Called each frame. The default implementation checks badguy state and
   * calls active_update and inactive_update
   */
  virtual void update(float elapsed_time);
  /** Called when a collision with another object occured. The default
   * implemetnation calls collision_player, collision_solid, collision_badguy
   * and collision_squished
   */
  virtual HitResponse collision(GameObject& other, const CollisionHit& hit);

  /** Called when a collision with tile with special attributes occured */
  virtual void collision_tile(uint32_t tile_attributes);

  /** Set the badguy to kill/falling state, which makes him falling of the
   * screen (his sprite is turned upside-down)
   */
  virtual void kill_fall();
  
  /** Call this, if you use custom kill_fall() or kill_squashed(GameObject& object) */
  virtual void run_dead_script();

  /** Writes out the badguy into the included lisp::Writer. Useful e.g. when
   * converting an old-format level to the new format.
   */
  virtual void write(lisp::Writer& writer);

  /**
   * True if this badguy can break bricks or open bonusblocks in his current form.
   */
  virtual bool can_break()
  {
    return false;
  }

  Vector get_start_position() const
  {
    return start_position;
  }
  void set_start_position(const Vector& vec)
  {
    start_position = vec;
  }

  /** Count this badguy to the statistics? This value should not be changed
   * during runtime. */
  bool countMe;

  /**
   * Called when hit by a fire bullet, and is_flammable() returns true
   */
  virtual void ignite();

  /**
   * Called to revert a badguy when is_ignited() returns true
   */
  virtual void extinguish();

  /**
   * Returns whether to call ignite() when a badguy gets hit by a fire bullet
   */
  virtual bool is_flammable() const;

  /**
   * Returns whether this badguys is currently on fire
   */
  bool is_ignited() const;

  /**
   * Called when hit by an ice bullet, and is_freezable() returns true.
   */
  virtual void freeze();

  /**
   * Called to unfreeze the badguy.
   */
  virtual void unfreeze();

  virtual bool is_freezable() const;

  bool is_frozen() const;

protected:
  enum State {
    STATE_INIT,
    STATE_INACTIVE,
    STATE_ACTIVE,
    STATE_SQUISHED,
    STATE_FALLING
  };

  /** Called when the badguy collided with a player */
  virtual HitResponse collision_player(Player& player, const CollisionHit& hit);
  /** Called when the badguy collided with solid ground */
  virtual void collision_solid(const CollisionHit& hit);
  /** Called when the badguy collided with another badguy */
  virtual HitResponse collision_badguy(BadGuy& other, const CollisionHit& hit);

  /** Called when the player hit the badguy from above. You should return true
   * if the badguy was squished, false if squishing wasn't possible
   */
  virtual bool collision_squished(GameObject& object);

  /** Called when the badguy collided with a bullet */
  virtual HitResponse collision_bullet(Bullet& bullet, const CollisionHit& hit);

  /** called each frame when the badguy is activated. */
  virtual void active_update(float elapsed_time);
  /** called each frame when the badguy is not activated. */
  virtual void inactive_update(float elapsed_time);

  /**
   * called when the badguy has been activated. (As a side effect the dir
   * variable might have been changed so that it faces towards the player.
   */
  virtual void activate();
  /** called when the badguy has been deactivated */
  virtual void deactivate();

  void kill_squished(GameObject& object);

  void set_state(State state);
  State get_state() const
  { return state; }

  /**
   * returns a pointer to the nearest player or 0 if no player is available
   */
  Player* get_nearest_player();

  /// is the enemy activated
  bool activated;
  /**
   * initial position of the enemy. Also the position where enemy respawns when
   * after being deactivated.
   */
  bool is_offscreen();
  /**
   *  Returns true if we might soon fall at least @c height pixels. Minimum
   *  value for height is 1 pixel
   */
  bool might_fall(int height = 1);

  Vector start_position;

  /**
   * The direction we currently face in
   */
  Direction dir;

  /**
   * The direction we initially faced in
   */
  Direction start_dir;

  /**
   *  Get Direction from String.
   */
  Direction str2dir( std::string dir_str );

  /**
   * Update on_ground_flag judging by solid collision @c hit.
   * This gets called from the base implementation of collision_solid, so call this when overriding collision_solid's default behaviour.
   */
  void update_on_ground_flag(const CollisionHit& hit);

  /**
   * Returns true if we touched ground in the past frame
   * This only works if update_on_ground_flag() gets called in collision_solid.
   */
  bool on_ground();

  /**
   * Returns floor normal stored the last time when update_on_ground_flag was called and we touched something solid from above.
   */
  Vector get_floor_normal();

  bool frozen;
  bool ignited; /**< true if this badguy is currently on fire */

  std::string dead_script; /**< script to execute when badguy is killed */

private:
  void try_activate();

  State state;
  Timer state_timer;
  bool on_ground_flag; /**< true if we touched something solid from above and update_on_ground_flag was called last frame */
  Vector floor_normal; /**< floor normal stored the last time when update_on_ground_flag was called and we touched something solid from above */

};

#endif
