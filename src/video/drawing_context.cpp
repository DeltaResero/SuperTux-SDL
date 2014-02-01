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

#include <algorithm>
#include <cassert>
#include <iostream>
#include <SDL_image.h>

#include "drawing_context.hpp"
#include "surface.hpp"
#include "font.hpp"
#include "main.hpp"
#include "gameconfig.hpp"
#include "glutil.hpp"
#include "texture.hpp"
#include "texture_manager.hpp"
#define LIGHTMAP_DIV 4

static inline int next_po2(int val)
{
  int result = 1;
  while(result < val)
    result *= 2;

  return result;
}

DrawingContext::DrawingContext()
  : ambient_color( 1.0f, 1.0f, 1.0f, 1.0f )
{
  screen = SDL_GetVideoSurface();

#ifdef HAVE_OPENGL
  lightmap_width = screen->w / LIGHTMAP_DIV;
  lightmap_height = screen->h / LIGHTMAP_DIV;
  unsigned int width = next_po2(lightmap_width);
  unsigned int height = next_po2(lightmap_height);

  lightmap = new Texture(width, height, GL_RGB);

  lightmap_uv_right = static_cast<float>(lightmap_width) / static_cast<float>(width);
  lightmap_uv_bottom = static_cast<float>(lightmap_height) / static_cast<float>(height);
  texture_manager->register_texture(lightmap);
#endif

  requests = &drawing_requests;
}

DrawingContext::~DrawingContext()
{
#ifdef HAVE_OPENGL
  texture_manager->remove_texture(lightmap);
  delete lightmap;
#endif
}

void
DrawingContext::draw_surface(const Surface* surface, const Vector& position,
                             float angle, const Color& color, const Blend& blend,
                             int layer)
{
  assert(surface != 0);

  DrawingRequest request;

  request.type = SURFACE;
  request.pos = transform.apply(position);

  if(request.pos.x >= SCREEN_WIDTH || request.pos.y >= SCREEN_HEIGHT
      || request.pos.x + surface->get_width() < 0
      || request.pos.y + surface->get_height() < 0)
    return;

  request.layer = layer;
  request.drawing_effect = transform.drawing_effect;
  request.alpha = transform.alpha;
  request.angle = angle;
  request.color = color;
  request.blend = blend;

  request.request_data = const_cast<Surface*> (surface);

  requests->push_back(request);
}

void
DrawingContext::draw_surface(const Surface* surface, const Vector& position,
    int layer)
{
  draw_surface(surface, position, 0.0f, Color(1.0f, 1.0f, 1.0f), Blend(), layer);
}

void
DrawingContext::draw_surface_part(const Surface* surface, const Vector& source,
    const Vector& size, const Vector& dest, int layer)
{
  assert(surface != 0);

  DrawingRequest request;

  request.type = SURFACE_PART;
  request.pos = transform.apply(dest);
  request.layer = layer;
  request.drawing_effect = transform.drawing_effect;
  request.alpha = transform.alpha;

  SurfacePartRequest* surfacepartrequest = new SurfacePartRequest();
  surfacepartrequest->size = size;
  surfacepartrequest->source = source;
  surfacepartrequest->surface = surface;

  // clip on screen borders
  if(request.pos.x < 0) {
    surfacepartrequest->size.x += request.pos.x;
    if(surfacepartrequest->size.x <= 0)
      return;
    surfacepartrequest->source.x -= request.pos.x;
    request.pos.x = 0;
  }
  if(request.pos.y < 0) {
    surfacepartrequest->size.y += request.pos.y;
    if(surfacepartrequest->size.y <= 0)
      return;
    surfacepartrequest->source.y -= request.pos.y;
    request.pos.y = 0;
  }
  request.request_data = surfacepartrequest;

  requests->push_back(request);
}

void
DrawingContext::draw_text(const Font* font, const std::string& text,
    const Vector& position, FontAlignment alignment, int layer)
{
  DrawingRequest request;

  request.type = TEXT;
  request.pos = transform.apply(position);
  request.layer = layer;
  request.drawing_effect = transform.drawing_effect;
  request.alpha = transform.alpha;

  TextRequest* textrequest = new TextRequest;
  textrequest->font = font;
  textrequest->text = text;
  textrequest->alignment = alignment;
  request.request_data = textrequest;

  requests->push_back(request);
}

void
DrawingContext::draw_center_text(const Font* font, const std::string& text,
    const Vector& position, int layer)
{
  draw_text(font, text, Vector(position.x + SCREEN_WIDTH/2, position.y),
      CENTER_ALLIGN, layer);
}

void
DrawingContext::draw_gradient(const Color& top, const Color& bottom, int layer)
{
  DrawingRequest request;

  request.type = GRADIENT;
  request.pos = Vector(0,0);
  request.layer = layer;

  request.drawing_effect = transform.drawing_effect;
  request.alpha = transform.alpha;

  GradientRequest* gradientrequest = new GradientRequest;
  gradientrequest->top = top;
  gradientrequest->bottom = bottom;
  request.request_data = gradientrequest;

  requests->push_back(request);
}

void
DrawingContext::draw_filled_rect(const Vector& topleft, const Vector& size,
                                 const Color& color, int layer)
{
  DrawingRequest request;

  request.type = FILLRECT;
  request.pos = transform.apply(topleft);
  request.layer = layer;

  request.drawing_effect = transform.drawing_effect;
  request.alpha = transform.alpha;

  FillRectRequest* fillrectrequest = new FillRectRequest;
  fillrectrequest->size = size;
  fillrectrequest->color = color;
  fillrectrequest->color.alpha = color.alpha * transform.alpha;
  request.request_data = fillrectrequest;

  requests->push_back(request);
}

void
DrawingContext::draw_filled_rect(const Rect& rect, const Color& color,
                                 int layer)
{
  DrawingRequest request;

  request.type = FILLRECT;
  request.pos = transform.apply(rect.p1);
  request.layer = layer;

  request.drawing_effect = transform.drawing_effect;
  request.alpha = transform.alpha;

  FillRectRequest* fillrectrequest = new FillRectRequest;
  fillrectrequest->size = Vector(rect.get_width(), rect.get_height());
  fillrectrequest->color = color;
  fillrectrequest->color.alpha = color.alpha * transform.alpha;
  request.request_data = fillrectrequest;

  requests->push_back(request);
}

void
DrawingContext::get_light(const Vector& position, Color* color)
{
  if( ambient_color.red == 1.0f && ambient_color.green == 1.0f
      && ambient_color.blue  == 1.0f ) {
    *color = Color( 1.0f, 1.0f, 1.0f);
    return;
  }
  DrawingRequest request;
  request.type = GETLIGHT;
  request.pos = transform.apply(position);
  request.layer = LAYER_GUI; //make sure all get_light requests are handled last.

  GetLightRequest* getlightrequest = new GetLightRequest;
  getlightrequest->color_ptr = color;
  request.request_data = getlightrequest;
  lightmap_requests.push_back(request);
}

void
DrawingContext::get_light(DrawingRequest& request)
{
  GetLightRequest* getlightrequest = (GetLightRequest*) request.request_data;

#ifdef HAVE_OPENGL
  if(config->use_opengl)
  {
  float pixels[3];
  for( int i = 0; i<3; i++)
    pixels[i] = 0.0f; //set to black

  //TODO: hacky. Make coordinate conversion more generic
  glReadPixels((GLint) request.pos.x / 4, 600-(GLint)request.pos.y / 4, 1, 1, GL_RGB, GL_FLOAT, pixels);
  *(getlightrequest->color_ptr) = Color( pixels[0], pixels[1], pixels[2]);
  //printf("get_light %f/%f r%f g%f b%f\n", request.pos.x, request.pos.y, pixels[0], pixels[1], pixels[2]);
  }
  else
#endif
  {
    // FIXME: implement properly for SDL
    static int i = 0;
    i += 1; i &= 0xFFFF;
    if (i & 0x8000) {
      *(getlightrequest->color_ptr) = Color(0.0f, 0.0f, 0.0f);
    } else {
      *(getlightrequest->color_ptr) = Color(1.0f, 1.0f, 1.0f);
    }
  }
  delete getlightrequest;
}

void
DrawingContext::draw_surface_part(DrawingRequest& request)
{
  SurfacePartRequest* surfacepartrequest
    = (SurfacePartRequest*) request.request_data;

  surfacepartrequest->surface->draw_part(
      surfacepartrequest->source.x, surfacepartrequest->source.y,
      request.pos.x, request.pos.y,
      surfacepartrequest->size.x, surfacepartrequest->size.y,
      request.alpha, request.drawing_effect);

  delete surfacepartrequest;
}

void
DrawingContext::draw_gradient(DrawingRequest& request)
{
  GradientRequest* gradientrequest = (GradientRequest*) request.request_data;
  const Color& top = gradientrequest->top;
  const Color& bottom = gradientrequest->bottom;

#ifdef HAVE_OPENGL
if(config->use_opengl)
{
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glColor4f(top.red, top.green, top.blue, top.alpha);
  glVertex2f(0, 0);
  glVertex2f(SCREEN_WIDTH, 0);
  glColor4f(bottom.red, bottom.green, bottom.blue, bottom.alpha);
  glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT);
  glVertex2f(0, SCREEN_HEIGHT);
  glEnd();
  glEnable(GL_TEXTURE_2D);

}
else
#endif
{
  for(int y = 0;y < screen->h;++y)
  {
    Uint8 r = (Uint8)((((float)(top.red-bottom.red)/(0-screen->h)) * y + top.red) * 255);
    Uint8 g = (Uint8)((((float)(top.green-bottom.green)/(0-screen->h)) * y + top.green) * 255);
    Uint8 b = (Uint8)((((float)(top.blue-bottom.blue)/(0-screen->h)) * y + top.blue) * 255);
    Uint8 a = (Uint8)((((float)(top.alpha-bottom.alpha)/(0-screen->h)) * y + top.alpha) * 255);
    Uint32 color = SDL_MapRGB(screen->format, r, g, b);

    SDL_Rect rect;
    rect.x = 0;
    rect.y = y;
    rect.w = screen->w;
    rect.h = 1;

    if(a == SDL_ALPHA_OPAQUE) {
      SDL_FillRect(screen, &rect, color);
    } else if(a != SDL_ALPHA_TRANSPARENT) {
      SDL_Surface *temp = SDL_CreateRGBSurface(screen->flags, rect.w, rect.h, screen->format->BitsPerPixel, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

      SDL_FillRect(temp, 0, color);
      SDL_SetAlpha(temp, SDL_SRCALPHA, a);
      SDL_BlitSurface(temp, 0, screen, &rect);
      SDL_FreeSurface(temp);
    }
  }
}
  delete gradientrequest;
}

void
DrawingContext::draw_text(DrawingRequest& request)
{
  TextRequest* textrequest = (TextRequest*) request.request_data;

  textrequest->font->draw(textrequest->text, request.pos,
      textrequest->alignment, request.drawing_effect, request.alpha);

  delete textrequest;
}

void
DrawingContext::draw_filled_rect(DrawingRequest& request)
{
  FillRectRequest* fillrectrequest = (FillRectRequest*) request.request_data;

#ifdef HAVE_OPENGL
if(config->use_opengl)
{
  float x = request.pos.x;
  float y = request.pos.y;
  float w = fillrectrequest->size.x;
  float h = fillrectrequest->size.y;

  glDisable(GL_TEXTURE_2D);
  glColor4f(fillrectrequest->color.red, fillrectrequest->color.green,
            fillrectrequest->color.blue, fillrectrequest->color.alpha);

  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x+w, y);
  glVertex2f(x+w, y+h);
  glVertex2f(x, y+h);
  glEnd();
  glEnable(GL_TEXTURE_2D);
}
else
#endif
{
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

  SDL_Rect rect;
  rect.x = (Sint16)request.pos.x * numerator / denominator;
  rect.y = (Sint16)request.pos.y * numerator / denominator;
  rect.w = (Uint16)fillrectrequest->size.x * numerator / denominator;
  rect.h = (Uint16)fillrectrequest->size.y * numerator / denominator;
  Uint8 r = static_cast<Uint8>(fillrectrequest->color.red * 255);
  Uint8 g = static_cast<Uint8>(fillrectrequest->color.green * 255);
  Uint8 b = static_cast<Uint8>(fillrectrequest->color.blue * 255);
  Uint8 a = static_cast<Uint8>(fillrectrequest->color.alpha * 255);
  Uint32 color = SDL_MapRGB(screen->format, r, g, b);
  if(a == SDL_ALPHA_OPAQUE) {
    SDL_FillRect(screen, &rect, color);
  } else if(a != SDL_ALPHA_TRANSPARENT) {
    SDL_Surface *temp = SDL_CreateRGBSurface(screen->flags, rect.w, rect.h, screen->format->BitsPerPixel, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);

    SDL_FillRect(temp, 0, color);
    SDL_SetAlpha(temp, SDL_SRCALPHA, a);
    SDL_BlitSurface(temp, 0, screen, &rect);
    SDL_FreeSurface(temp);
  }
}
  delete fillrectrequest;
}

void
#ifdef HAVE_OPENGL
DrawingContext::draw_lightmap(DrawingRequest& request)
#else
DrawingContext::draw_lightmap(DrawingRequest&)
#endif
{
#ifdef HAVE_OPENGL
  const Texture* texture = reinterpret_cast<Texture*> (request.request_data);

  // multiple the lightmap with the framebuffer
  glBlendFunc(GL_DST_COLOR, GL_ZERO);

  glBindTexture(GL_TEXTURE_2D, texture->get_handle());
  glBegin(GL_QUADS);

  glTexCoord2f(0, lightmap_uv_bottom);
  glVertex2f(0, 0);

  glTexCoord2f(lightmap_uv_right, lightmap_uv_bottom);
  glVertex2f(SCREEN_WIDTH, 0);

  glTexCoord2f(lightmap_uv_right, 0);
  glVertex2f(SCREEN_WIDTH, SCREEN_HEIGHT);

  glTexCoord2f(0, 0);
  glVertex2f(0, SCREEN_HEIGHT);

  glEnd();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

void
DrawingContext::do_drawing()
{
#ifdef DEBUG
  assert(transformstack.empty());
  assert(target_stack.empty());
#endif
  transformstack.clear();
  target_stack.clear();

  //Use Lightmap if ambient color is not white.
  bool use_lightmap = ( ambient_color.red != 1.0f   || ambient_color.green != 1.0f ||
                        ambient_color.blue  != 1.0f );

  // PART1: create lightmap
  if(use_lightmap) {
#ifdef HAVE_OPENGL
  if(config->use_opengl)
  {
    glViewport(0, screen->h - lightmap_height, lightmap_width, lightmap_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor( ambient_color.red, ambient_color.green, ambient_color.blue, 1 );
    glClear(GL_COLOR_BUFFER_BIT);
    handle_drawing_requests(lightmap_requests);
    lightmap_requests.clear();

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, lightmap->get_handle());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, screen->h - lightmap_height, lightmap_width, lightmap_height);

    glViewport(0, 0, screen->w, screen->h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_BLEND);
  }
  else
#endif
  {
    // FIXME: SDL alternative
  }

    // add a lightmap drawing request into the queue
    DrawingRequest request;
    request.type = LIGHTMAPREQUEST;
    request.layer = LAYER_HUD - 1;
    request.request_data = lightmap;
    requests->push_back(request);
  }

  //glClear(GL_COLOR_BUFFER_BIT);
  handle_drawing_requests(drawing_requests);
  drawing_requests.clear();
#ifdef HAVE_OPENGL
  if(config->use_opengl)
  {
    assert_gl("drawing");
  }
#endif

#ifdef HAVE_OPENGL
  if(config->use_opengl)
  {
    SDL_GL_SwapBuffers();
  }
  else
#endif
  {
    SDL_Flip(screen);
  }
}

void
DrawingContext::handle_drawing_requests(DrawingRequests& requests)
{
  std::stable_sort(requests.begin(), requests.end());

  for(DrawingRequests::iterator i = requests.begin();
      i != requests.end(); ++i) {
    switch(i->type) {
      case SURFACE:
      {
        const Surface* surface = (const Surface*) i->request_data;
        if (i->angle == 0.0f && 
            i->color.red == 1.0f && i->color.green == 1.0f  &&
            i->color.blue == 1.0f &&  i->color.alpha == 1.0f  )
          surface->draw(i->pos.x, i->pos.y, i->alpha, i->drawing_effect);
        else
          surface->draw(i->pos.x, i->pos.y, i->alpha, i->angle, i->color, i->blend, i->drawing_effect);
        break;
      }
      case SURFACE_PART:
        draw_surface_part(*i);
        break;
      case GRADIENT:
        draw_gradient(*i);
        break;
      case TEXT:
        draw_text(*i);
        break;
      case FILLRECT:
        draw_filled_rect(*i);
        break;
      case LIGHTMAPREQUEST:
        draw_lightmap(*i);
        break;
      case GETLIGHT:
        get_light(*i);
        break;
    }
  }
}

void
DrawingContext::push_transform()
{
  transformstack.push_back(transform);
}

void
DrawingContext::pop_transform()
{
  assert(!transformstack.empty());

  transform = transformstack.back();
  transformstack.pop_back();
}

void
DrawingContext::set_drawing_effect(DrawingEffect effect)
{
  transform.drawing_effect = effect;
}

DrawingEffect
DrawingContext::get_drawing_effect() const
{
  return transform.drawing_effect;
}

void
DrawingContext::set_alpha(float alpha)
{
  transform.alpha = alpha;
}

float
DrawingContext::get_alpha() const
{
  return transform.alpha;
}

void
DrawingContext::push_target()
{
  target_stack.push_back(target);
}

void
DrawingContext::pop_target()
{
  set_target(target_stack.back());
  target_stack.pop_back();
}

void
DrawingContext::set_target(Target target)
{
  this->target = target;
  if(target == LIGHTMAP)
    requests = &lightmap_requests;
  else
    requests = &drawing_requests;
}

void
DrawingContext::set_ambient_color( Color new_color )
{
  ambient_color = new_color;
}
