//  $Id$
//
//  SuperTux
//  Copyright (C) 2006 Ondrej Hosek <ondra.hosek@gmail.com>
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

#include "falling_coin.hpp"
#include "player.hpp"
#include "sprite/sprite_manager.hpp"
#include "resources.hpp"
#include "main.hpp"

FallingCoin::FallingCoin(const Vector& start_position, const int vel_x)
{
  pos = start_position;
  sprite = sprite_manager->create("images/objects/coin/coin.sprite");
  physic.set_velocity_y(-640);
  physic.set_velocity_x(vel_x);
}

FallingCoin::~FallingCoin()
{
  delete sprite;
}

void
FallingCoin::draw(DrawingContext& context)
{
  sprite->draw(context, pos, LAYER_FLOATINGOBJECTS + 5);
}

void
FallingCoin::update(float elapsed_time)
{
  pos += physic.get_movement(elapsed_time);
  if (pos.y > SCREEN_HEIGHT)
    remove_me();
}
