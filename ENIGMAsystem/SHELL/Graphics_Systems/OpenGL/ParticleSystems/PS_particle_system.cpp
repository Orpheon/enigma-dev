/********************************************************************************\
**                                                                              **
**  Copyright (C) 2012-2013 forthevin                                           **
**                                                                              **
**  This file is a part of the ENIGMA Development Environment.                  **
**                                                                              **
**                                                                              **
**  ENIGMA is free software: you can redistribute it and/or modify it under the **
**  terms of the GNU General Public License as published by the Free Software   **
**  Foundation, version 3 of the license or any later version.                  **
**                                                                              **
**  This application and its source code is distributed AS-IS, WITHOUT ANY      **
**  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS   **
**  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more       **
**  details.                                                                    **
**                                                                              **
**  You should have recieved a copy of the GNU General Public License along     **
**  with this code. If not, see <http://www.gnu.org/licenses/>                  **
**                                                                              **
**  ENIGMA is an environment designed to create games and other programs with a **
**  high-level, fully compilable language. Developers of ENIGMA or anything     **
**  associated with ENIGMA are in no way responsible for its users or           **
**  applications created by its users, or damages caused by the environment     **
**  or programs made in the environment.                                        **
**                                                                              **
\********************************************************************************/

#include "PS_particle_system.h"
#include "PS_particle.h"
#include "PS_particle_type.h"
#include "PS_particle_instance.h"
#include "PS_particle_emitter.h"
#include "PS_particle_attractor.h"
#include "PS_particle_destroyer.h"
#include "PS_particle_deflector.h"
#include "PS_particle_changer.h"
#include "Universal_System/depth_draw.h"
#include "Graphics_Systems/OpenGL/GSstdraw.h"
#include "Graphics_Systems/OpenGL/GScolors.h"
#include "Graphics_Systems/OpenGL/GSsprite.h"
#include "Graphics_Systems/OpenGL/binding.h"
#include "Universal_System/spritestruct.h"
#include <GL/gl.h>
#include <cmath>
#include <cstdlib>
#include <list>
#include <vector>

#define __GETR(x) ((x & 0x0000FF))
#define __GETG(x) ((x & 0x00FF00) >> 8)
#define __GETB(x) ((x & 0xFF0000) >> 16)

inline int bounds(int value, int low, int high)
{
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

using enigma::pt_manager;

namespace enigma
{
  struct generation_info
  {
    double x;
    double y;
    int number;
    particle_type* pt;
  };
  struct particle_system
  {
    // Wiggling.
    double wiggle;
    int wiggle_frequency; // Number of steps for a full cycle. Domain: [1;[.
    double get_wiggle_result(double wiggle_offset);
    // Subimage index.
    int subimage_index;
    // Particles.
    int id;
    bool oldtonew;
    double x_offset, y_offset;
    double depth; // Integer stored as double.
    std::list<particle_instance> pi_list;
    bool auto_update, auto_draw;
    void initialize();
    void update_particlesystem();
    void draw_particle(particle_instance* it);
    void draw_particlesystem();
    void create_particles(double x, double y, particle_type* pt, int number, bool use_color=false, int given_color=c_white);
    // Emitters.
    std::map<int,particle_emitter*> id_to_emitter;
    int emitter_max_id;
    int create_emitter();
    void set_emitter_region(int em_id, double xmin, double xmax, double ymin, double ymax, ps_shape shape, ps_distr distribution);
    void set_emitter_stream(int em_id, int particle_type_id, int number);
    // Attractors.
    std::map<int,particle_attractor*> id_to_attractor;
    int attractor_max_id;
    int create_attractor();
    // Destroyers.
    std::map<int,particle_destroyer*> id_to_destroyer;
    int destroyer_max_id;
    int create_destroyer();
    // Deflectors.
    std::map<int,particle_deflector*> id_to_deflector;
    int deflector_max_id;
    int create_deflector();
    // Changers.
    std::map<int,particle_changer*> id_to_changer;
    int changer_max_id;
    int create_changer();
  };
  double particle_system::get_wiggle_result(double wiggle_offset)
  {
    double result_wiggle = wiggle + wiggle_offset;
    result_wiggle = result_wiggle > 1.0 ? result_wiggle - 1.0 : result_wiggle;
    if (result_wiggle < 0.5) return -1.0 + 4*result_wiggle;
    else return 3.0 - 4.0*result_wiggle;
  }
  void particle_system::initialize()
  {
    wiggle = 0;
    wiggle_frequency = 25;
    oldtonew = true;
    auto_update = true, auto_draw = true;
    depth = 0.0;
    pi_list = std::list<particle_instance>();
    id_to_emitter = std::map<int,particle_emitter*>();
    emitter_max_id = 0;
    id_to_attractor = std::map<int,particle_attractor*>();
    attractor_max_id = 0;
    id_to_destroyer = std::map<int,particle_destroyer*>();
    destroyer_max_id = 0;
    id_to_deflector = std::map<int,particle_deflector*>();
    deflector_max_id = 0;
    id_to_changer = std::map<int,particle_changer*>();
    changer_max_id = 0;
  }
  void particle_system::update_particlesystem()
  {
    // Increase wiggle.
    wiggle += 1.0/wiggle_frequency;
    if (wiggle > 1.0) {
      wiggle -= 1.0;
    }
    // Increase subimage_index.
    subimage_index++;

    std::vector<generation_info> particles_to_generate;
    // Handle life and death.
    {
      for (std::list<particle_instance>::iterator it = pi_list.begin(); it != pi_list.end(); it++)
      {
        // Decrease life.
        it->life_current--;
        if (it->life_current <= 0) { // Death.
          particle_type* pt = it->pt;

          // Generated upon end of life.
          if (pt->alive && pt->death_on) {
            std::map<int,particle_type*>::iterator death_pt_it = pt_manager.id_to_particletype.find(pt->death_particle_id);
            if (death_pt_it != pt_manager.id_to_particletype.end()) {
              generation_info gen_info;
              gen_info.x = it->x;
              gen_info.y = it->y;
              gen_info.number = pt->death_number;
              gen_info.pt = (*death_pt_it).second;
              particles_to_generate.push_back(gen_info);
            }
          }

          // Death handling.
          pt->particle_count--;
          if (pt->particle_count <= 0 && !pt->alive) {
            // Particle type is no longer used, delete it.
            int id = pt->id;
            delete pt;
            enigma::pt_manager.id_to_particletype.erase(id);
          }
          it = pi_list.erase(it);
        }
      }
    }
    // Shape.
    {
      std::list<particle_instance>::iterator end = pi_list.end();
      for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
      {
        particle_type* pt = it->pt;
        if (pt->alive) {
          it->size = std::max(it->size + pt->size_incr, 0.0);
          it->angle = fmod(it->angle + pt->ang_incr, 360.0);
        }
      }
    }
    // Color and blending.
    {
      std::list<particle_instance>::iterator end = pi_list.end();
      for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
      {
        particle_type* pt = it->pt;
        // Color.
        switch(pt->c_mode) {
        case one_color : {break;}
        case two_color : {
          if (pt->alive) {
            const int r1 = color_get_red(pt->color1),
                g1 = color_get_green(pt->color1),
                b1 = color_get_blue(pt->color1);
            const int r2 = color_get_red(pt->color2),
                g2 = color_get_green(pt->color2),
                b2 = color_get_blue(pt->color2);
            const double part = 1.0 - 1.0*it->life_current/it->life_start;
            it->color = make_color_rgb(int((1-part)*r1 + part*r2),int((1-part)*g1 + part*g2),int((1-part)*b1 + part*b2));
          }
          break;
        }
        case three_color : {
          if (pt->alive) {
            double part = 1.0 - 1.0*it->life_current/it->life_start;
            int first_color, second_color;
            if (part <= 0.5) {
              part = 2.0*part;
              first_color = pt->color1;
              second_color = pt->color2;
            }
            else {
              part = 2.0*(part - 0.5);
              first_color = pt->color2;
              second_color = pt->color3;
            }
            const int r1 = color_get_red(first_color),
                g1 = color_get_green(first_color),
                b1 = color_get_blue(first_color);
            const int r2 = color_get_red(second_color),
                g2 = color_get_green(second_color),
                b2 = color_get_blue(second_color);
            it->color = make_color_rgb(int((1-part)*r1 + part*r2),int((1-part)*g1 + part*g2),int((1-part)*b1 + part*b2));
          }
          break;
        }
        case mix_color : {break;}
        case rgb_color : {break;}
        case hsv_color : {break;}
        }
        // Alpha.
        switch(pt->a_mode) {
        case one_alpha : {break;}
        case two_alpha : {
          if (pt->alive) {
            const int alpha1 = pt->alpha1;
            const int alpha2 = pt->alpha2;
            const double part = 1.0 - 1.0*it->life_current/it->life_start;
            it->alpha = bounds(int((1-part)*alpha1 + part*alpha2), 0, 255);
          }
          break;
        }
        case three_alpha : {
          if (pt->alive) {
            const int alpha1 = pt->alpha1;
            const int alpha2 = pt->alpha2;
            const int alpha3 = pt->alpha3;
            double part = 1.0 - 1.0*it->life_current/it->life_start;
            int first_alpha, second_alpha;
            if (part <= 0.5) {
              part = 2.0*part;
              first_alpha = alpha1;
              second_alpha = alpha2;
            }
            else {
              part = 2.0*(part - 0.5);
              first_alpha = alpha2;
              second_alpha = alpha3;
            }
            it->alpha = bounds(int((1-part)*first_alpha + part*second_alpha), 0, 255);
          }
          break;
        }
        }
      }
    }
    // Step.
    {
      for (std::list<particle_instance>::iterator it = pi_list.begin(); it != pi_list.end(); it++)
      {
        particle_type* pt = it->pt;

        // Generated each step.
        if (pt->alive && pt->step_on) {
          std::map<int,particle_type*>::iterator step_pt_it = pt_manager.id_to_particletype.find(pt->step_particle_id);
          if (step_pt_it != pt_manager.id_to_particletype.end()) {
            generation_info gen_info;
            gen_info.x = it->x;
            gen_info.y = it->y;
            gen_info.number = pt->step_number;
            gen_info.pt = (*step_pt_it).second;
            particles_to_generate.push_back(gen_info);
          }
        }
      }
    }
    // Move particles.
    {
      std::list<particle_instance>::iterator end = pi_list.end();
      for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
      {
        particle_type* pt = it->pt;
        if (pt->alive) {
          it->speed += pt->speed_incr;
          it->direction += pt->dir_incr;
          if (it->speed < 0) {
            it->speed = -it->speed;
            it->direction += 180.0;
          }
          it->direction = fmod(it->direction, 360.0);
          const double speed = it->speed, direction = it->direction;
          const double grav_amount = pt->grav_amount, grav_dir = pt->grav_dir;
          const double vx = speed*cos(direction*M_PI/180.0) + grav_amount*cos(grav_dir*M_PI/180.0);
          const double vy = -(speed*sin(direction*M_PI/180.0) + grav_amount*sin(grav_dir*M_PI/180.0));
          it->speed = sqrt(vx*vx + vy*vy);
          it->direction = vx == 0 && vy == 0 ? direction : -atan2(vy,vx)*180.0/M_PI;
        }
        double speed = it->speed, direction = it->direction;
        if (pt->alive) {
          speed += pt->speed_wiggle*get_wiggle_result(it->speed_wiggle_offset);
          direction += pt->dir_wiggle*get_wiggle_result(it->dir_wiggle_offset);
        }
        it->x += speed*cos(direction*M_PI/180.0);
        it->y += -speed*sin(direction*M_PI/180.0);
      }
    }
    // Changers.
    {
      std::map<int,particle_changer*>::iterator end = id_to_changer.end();
      for (std::map<int,particle_changer*>::iterator ch_it = id_to_changer.begin(); ch_it != end; ch_it++)
      {
        particle_changer* p_ch = (*ch_it).second;
        particle_type* pt1;
        particle_type* pt2;
        std::map<int,enigma::particle_type*>::iterator pt_it1 = enigma::pt_manager.id_to_particletype.find(p_ch->parttypeid1);
        if (pt_it1 == enigma::pt_manager.id_to_particletype.end()) {
          continue;
        }
        std::map<int,enigma::particle_type*>::iterator pt_it2 = enigma::pt_manager.id_to_particletype.find(p_ch->parttypeid2);
        if (pt_it2 == enigma::pt_manager.id_to_particletype.end()) {
          continue;
        }
        pt1 = (*pt_it1).second;
        pt2 = (*pt_it2).second;

        std::list<particle_instance>::iterator end = pi_list.end();
        for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
        {
          if (p_ch->is_inside(it->x, it->y) && it->pt->id == pt1->id) {
            // Store position of old particle.
            const double x = it->x, y = it->y;
            // Destroy the old particle.
            pt1->particle_count--;
            if (pt1->particle_count <= 0 && !pt1->alive) {
              // Particle type is no longer used, delete it.
              int id = pt1->id;
              delete pt1;
              enigma::pt_manager.id_to_particletype.erase(id);
            }
            it = pi_list.erase(it);
            // Create a new particle at its position.
            generation_info gen_info;
            gen_info.x = x;
            gen_info.y = y;
            gen_info.number = 1;
            gen_info.pt = pt2;
            particles_to_generate.push_back(gen_info);
          }
        }
      }
    }
    // Generate particles.
    for (std::vector<generation_info>::iterator it = particles_to_generate.begin(); it != particles_to_generate.end(); it++)
    {
      double x = (*it).x, y = (*it).y;
      int number = (*it).number;
      particle_type* pt = (*it).pt;
      number = number >= 0 ? number : (rand() % (-number) < 1 ? 1 : 0); // Create particle with probability -1/number.
      create_particles(x, y, pt, number);
    }
    // Emitters.
    {
      std::map<int,particle_emitter*>::iterator end = id_to_emitter.end();
      for (std::map<int,particle_emitter*>::iterator it = id_to_emitter.begin(); it != end; it++)
      {
        particle_emitter* p_e = (*it).second;
        std::map<int,particle_type*>::iterator pt_it = pt_manager.id_to_particletype.find(p_e->particle_type_id);
        if (pt_it != pt_manager.id_to_particletype.end()) {
          particle_type* p_t = (*pt_it).second;
          if (!p_t->alive) continue;
          const int number = p_e->get_step_number();
          for (int i = 1; i <= number; i++)
          {
            int x, y;
            p_e->get_point(x, y);
            create_particles(x, y, p_t, 1);
          }
        }
      }
    }
    // Attractors.
    {
      std::map<int,particle_attractor*>::iterator end = id_to_attractor.end();
      for (std::map<int,particle_attractor*>::iterator at_it = id_to_attractor.begin(); at_it != end; at_it++)
      {
        particle_attractor* p_a = (*at_it).second;
        std::list<particle_instance>::iterator end = pi_list.end();
        for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
        {
          // If the particle is not inside the attractor range of influence,
          // or is at the attractor's exact position,
          // skip to next attractor. 
          const double dx = it->x - p_a->x;
          const double dy = it->y - p_a->y;
          const double relative_distance = sqrt(dx*dx + dy*dy)/std::max(1.0, p_a->dist_effect);
          if (relative_distance > 1.0 || (dx == 0 && dy == 0)) {
            continue;
          }
          const double direction_radians = atan2(-(p_a->y - it->y), p_a->x - it->x);
          // Determine force.
          double force_effective_strength;
          switch (p_a->force_kind)  {
          case ps_fo_constant : force_effective_strength = p_a->force_strength; break;
          case ps_fo_linear : force_effective_strength = (1.0 - relative_distance)*p_a->force_strength; break;
          case ps_fo_quadratic : force_effective_strength = (1.0 - relative_distance)*(1.0 - relative_distance)*p_a->force_strength; break;
          default : force_effective_strength = p_a->force_strength; break;
          }
          // Apply force.
          if (p_a->additive) {
            const double vx = it->speed*cos(it->direction*M_PI/180.0) + force_effective_strength*cos(direction_radians);
            const double vy = -it->speed*sin(it->direction*M_PI/180.0) - force_effective_strength*sin(direction_radians);
            it->speed = sqrt(vx*vx + vy*vy);
            const double direction = it->direction;
            it->direction = vx == 0.0 && vy == 0.0 ? direction : -atan2(vy,vx)*180.0/M_PI;
          }
          else {
            it->x += force_effective_strength*cos(direction_radians);
            it->y += -force_effective_strength*sin(direction_radians);
          }
        }
      }
    }
    // Destroyers.
    {
      std::map<int,particle_destroyer*>::iterator end = id_to_destroyer.end();
      for (std::map<int,particle_destroyer*>::iterator ds_it = id_to_destroyer.begin(); ds_it != end; ds_it++)
      {
        particle_destroyer* p_ds = (*ds_it).second;
        std::list<particle_instance>::iterator end = pi_list.end();
        for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
        {
          if (p_ds->is_inside(it->x, it->y)) {
            particle_type* pt = it->pt;

            // Death handling.
            pt->particle_count--;
            if (pt->particle_count <= 0 && !pt->alive) {
              // Particle type is no longer used, delete it.
              int id = pt->id;
              delete pt;
              enigma::pt_manager.id_to_particletype.erase(id);
            }
            it = pi_list.erase(it);
          }
        }
      }
    }
    // Deflectors.
    {
      std::map<int,particle_deflector*>::iterator end = id_to_deflector.end();
      for (std::map<int,particle_deflector*>::iterator df_it = id_to_deflector.begin(); df_it != end; df_it++)
      {
        particle_deflector* p_df = (*df_it).second;
        std::list<particle_instance>::iterator end = pi_list.end();
        for (std::list<particle_instance>::iterator it = pi_list.begin(); it !=end; it++)
        {
          if (p_df->is_inside(it->x, it->y)) {
            particle_type* pt = it->pt;

            // Direction changing.
            it->direction = fmod(it->direction + 360.0, 360.0);
            switch (p_df->deflection_kind) {
            case ps_de_horizontal : {
              it->direction = it->direction <= 180.0 ? 180.0 - it->direction : 540.0 - it->direction;
              break;
            }
            case ps_de_vertical : {
              it->direction = 360.0 - it->direction;
              break;
            }
            default : {
              break;
            }
            }
            // Friction handling.
            const double new_speed = std::max(0.0, it->speed - p_df->friction);
            const double friction_effect = it->speed - new_speed;
            it->speed = new_speed;
            // Move one step.
            it->x += friction_effect*cos(it->direction*M_PI/180.0);
            it->y += -friction_effect*sin(it->direction*M_PI/180.0);
          }
        }
      }
    }
  }
  void particle_system::draw_particle(particle_instance* it)
  {
    {
      int color = it->color;
      int alpha = it->alpha;
      if (it->pt->alive) {  
        particle_type* pt = it->pt;

        double size;
        double rot_degrees;
        size = std::max(0.0, it->size + pt->size_wiggle*get_wiggle_result(it->size_wiggle_offset));
        rot_degrees = it->angle + pt->ang_wiggle*get_wiggle_result(it->ang_wiggle_offset);
        if (pt->ang_relative) {
          rot_degrees += it->direction;
        }

        if (size <= 0) return; // NOTE: Skip to next particle.

        if (!pt->is_particle_sprite) { // Draw sprite.
          int sprite_id = pt->sprite_id;
          int subimg;
          if (!pt->sprite_animated) {
            subimg = it->sprite_subimageindex_initial;
          }
          else {
            const enigma::sprite *const spr = enigma::spritestructarray[pt->sprite_id];
            const int subimage_count = spr->subcount;
            if (pt->sprite_stretched) {
              subimg = int(subimage_count*(1.0 - 1.0*it->life_current/it->life_start));
              subimg = subimg >= subimage_count ? subimage_count - 1 : subimg;
              subimg = subimg % subimage_count;
            }
            else {
              subimg = (subimage_index + it->sprite_subimageindex_initial) % subimage_count;
            }
          }
          const double x = it->x, y = it->y;
          const double xscale = pt->xscale*size, yscale = pt->yscale*size;

          glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT); // Push 1.
          if (pt->blend_additive) {
            glBlendFunc(GL_SRC_ALPHA,GL_ONE);
          }
          else {
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
          }
          draw_sprite_ext(sprite_id, subimg, x, y, xscale, yscale, rot_degrees, color, alpha/255.0);
          glPopAttrib(); // Pop 1.
        }
        else { // Draw particle sprite.

          particle_sprite* ps = pt->part_sprite;
          bind_texture(ps->texture);

          glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT); // Push 1.

          if (pt->blend_additive) {
            glBlendFunc(GL_SRC_ALPHA,GL_ONE);
          }
          else {
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
          }

          glColor4ub(__GETR(color),__GETG(color),__GETB(color), alpha);

          const double rot = rot_degrees*M_PI/180.0;

          const double x = it->x, y = it->y;
          const double xscale = pt->xscale*size, yscale = pt->yscale*size;

          const float
          w = ps->width*xscale, h = ps->height*yscale,
          tbx = 1, tby = 1,
          wsinrot = w*sin(rot), wcosrot = w*cos(rot);

          glBegin(GL_TRIANGLE_STRIP);

          float
          ulcx = x - xscale * (ps->width/2.0) * cos(rot) + yscale * (ps->height/2.0) * cos(M_PI/2+rot),
          ulcy = y + xscale * (ps->width/2.0) * sin(rot) - yscale * (ps->height/2.0) * sin(M_PI/2+rot);
          glTexCoord2f(0,0);
          glVertex2f(ulcx,ulcy);
          glTexCoord2f(tbx,0);
          glVertex2f(ulcx + wcosrot, ulcy - wsinrot);

          const double mpr = 3*M_PI/2 + rot;
          ulcx += h * cos(mpr);
          ulcy -= h * sin(mpr);
          glTexCoord2f(0,tby);
          glVertex2f(ulcx,ulcy);
          glTexCoord2f(tbx,tby);
          glVertex2f(ulcx + wcosrot, ulcy - wsinrot);

          glEnd();

          glPopAttrib(); // Pop 1.
        }
      }
      else { // Draw particle in a limited way if particle type not alive.
        double size = it->size;
        double rot_degrees = it->angle;
        if (size <= 0) return; // NOTE: Skip to next particle.

        particle_sprite* ps = enigma::get_particle_sprite(enigma::pt_sh_pixel);
        if (ps == NULL) return; // NOTE: Skip to next particle.
        bind_texture(ps->texture);

        glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT); // Push 1.

        glColor4ub(__GETR(color),__GETG(color),__GETB(color), alpha);

        const double rot = rot_degrees*M_PI/180.0;

        const double x = round(it->x), y = round(it->y);
        const double xscale = size, yscale = size;

        const float
        w = ps->width*xscale, h = ps->height*yscale,
        tbx = 1, tby = 1,
        wsinrot = w*sin(rot), wcosrot = w*cos(rot);

        glBegin(GL_TRIANGLE_STRIP);

        float
        ulcx = x - xscale * (ps->width/2.0) * cos(rot) + yscale * (ps->height/2.0) * cos(M_PI/2+rot),
        ulcy = y + xscale * (ps->width/2.0) * sin(rot) - yscale * (ps->height/2.0) * sin(M_PI/2+rot);
        glTexCoord2f(0,0);
        glVertex2f(ulcx,ulcy);
        glTexCoord2f(tbx,0);
        glVertex2f(ulcx + wcosrot, ulcy - wsinrot);

        const double mpr = 3*M_PI/2 + rot;
        ulcx += h * cos(mpr);
        ulcy -= h * sin(mpr);
        glTexCoord2f(0,tby);
        glVertex2f(ulcx,ulcy);
        glTexCoord2f(tbx,tby);
        glVertex2f(ulcx + wcosrot, ulcy - wsinrot);

        glEnd();

        glPopAttrib(); // Pop 1.
      }
    }
  }
  void particle_system::draw_particlesystem()
  {
    glPushMatrix(); // Push 1.

    glTranslated(x_offset, y_offset, 0.0);

    // Draw the particle system either from oldest to youngest or reverse.
    if (oldtonew) {
      const std::list<particle_instance>::iterator end = pi_list.end();
      for (std::list<particle_instance>::iterator it = pi_list.begin(); it != end; it++)
      {
        draw_particle(&(*it));
      }
    }
    else {
      const std::list<particle_instance>::reverse_iterator rend = pi_list.rend();
      for (std::list<particle_instance>::reverse_iterator it = pi_list.rbegin(); it != rend; it++)
      {
        draw_particle(&(*it));
      }
    }

    glPopMatrix(); // Pop 1.
  }
  void particle_system::create_particles(double x, double y, particle_type* pt, int number, bool use_color, int given_color)
  {
    if (number > 0) {
      pt->particle_count += number;
    }
    for (int i = 0; i < number; i++)
    {
      particle_instance pi;
      pi.pt = pt;
      // Shape.
      if (!pt->is_particle_sprite) {
        const enigma::sprite *const spr = enigma::spritestructarray[pt->sprite_id];
        const int subimage_count = spr->subcount;
        int subimageindex_initial;
        if (pt->sprite_random) {
          subimageindex_initial = int(subimage_count*1.0*rand()/(RAND_MAX-1)) % subimage_count;
        }
        else {
          subimageindex_initial = 0;
        }
        pi.sprite_subimageindex_initial = subimageindex_initial;
      }
      else {
        pi.sprite_subimageindex_initial = -1;
      }
      pi.size = pt->size_min + (pt->size_max-pt->size_min)*1.0*rand()/(RAND_MAX-1);
      pi.size_wiggle_offset = 1.0*rand()/(RAND_MAX-1);
      pi.angle = pt->ang_min + (pt->ang_max-pt->ang_min)*1.0*rand()/(RAND_MAX-1);
      pi.ang_wiggle_offset = 1.0*rand()/(RAND_MAX-1);
      // Color and blending.
      pi.color = pt->color1;
      switch(pt->c_mode) {
      case one_color : {
        if (use_color) {
          pi.color = given_color;
          break;
        }
        break;
      }
      case two_color : {break;}
      case three_color : {break;}
      case mix_color : {
        if (use_color) {
          pi.color = given_color;
          break;
        }
        double random_fact = 1.0*rand()/(RAND_MAX-1);
        unsigned char r = bounds(pt->rmin + (pt->rmax - pt->rmin)*random_fact, 0, 255);
        unsigned char g = bounds(pt->gmin + (pt->gmax - pt->gmin)*random_fact, 0, 255);
        unsigned char b = bounds(pt->bmin + (pt->bmax - pt->bmin)*random_fact, 0, 255);
        pi.color = make_color_rgb(r, g, b);
        break;
      }
      case rgb_color : {
        if (use_color) {
          pi.color = given_color;
          break;
        }
        unsigned char r = bounds(pt->rmin + (pt->rmax - pt->rmin)*1.0*rand()/(RAND_MAX-1), 0, 255);
        unsigned char g = bounds(pt->gmin + (pt->gmax - pt->gmin)*1.0*rand()/(RAND_MAX-1), 0, 255);
        unsigned char b = bounds(pt->bmin + (pt->bmax - pt->bmin)*1.0*rand()/(RAND_MAX-1), 0, 255);
        pi.color = make_color_rgb(r, g, b);
        break;
      }
      case hsv_color : {
        if (use_color) {
          pi.color = given_color;
          break;
        }
        unsigned char h = bounds(pt->hmin + (pt->hmax - pt->hmin)*1.0*rand()/(RAND_MAX-1), 0, 255);
        unsigned char s = bounds(pt->smin + (pt->smax - pt->smin)*1.0*rand()/(RAND_MAX-1), 0, 255);
        unsigned char v = bounds(pt->vmin + (pt->vmax - pt->vmin)*1.0*rand()/(RAND_MAX-1), 0, 255);
        pi.color = make_color_hsv(h, s, v);
        break;
      }
      }
      pi.alpha = pt->alpha1;
      // Life and death.
      pi.life_current = pt->life_min == pt->life_max ? pt->life_min : pt->life_min + rand() % (	pt->life_max - pt->life_min);
      pi.life_start = pi.life_current;
      // Motion.
      pi.x = x;
      pi.y = y;
      pi.speed = pt->speed_min + (pt->speed_max-pt->speed_min)*1.0*rand()/(RAND_MAX-1);
      pi.direction = pt->dir_min + (pt->dir_max-pt->dir_min)*1.0*rand()/(RAND_MAX-1);
      pi.speed_wiggle_offset = 1.0*rand()/(RAND_MAX-1);
      pi.dir_wiggle_offset = 1.0*rand()/(RAND_MAX-1);
      pi_list.push_back(pi);
    }
  }
  int particle_system::create_emitter()
  {
    particle_emitter* p_e = new particle_emitter();
    p_e->initialize();

    emitter_max_id++;
    id_to_emitter.insert(std::pair<int,particle_emitter*>(emitter_max_id, p_e));

    return emitter_max_id;
  }
  void particle_system::set_emitter_region(int em_id, double xmin, double xmax, double ymin, double ymax, ps_shape shape, ps_distr distribution)
  {
    std::map<int,particle_emitter*>::iterator pe_it = id_to_emitter.find(em_id);
    if (pe_it != id_to_emitter.end()) {
      (*pe_it).second->set_region(xmin, xmax, ymin, ymax, shape, distribution);
    }
  }
  void particle_system::set_emitter_stream(int em_id, int particle_type_id, int number)
  {
    std::map<int,particle_emitter*>::iterator pe_it = id_to_emitter.find(em_id);
    if (pe_it != id_to_emitter.end()) {
      (*pe_it).second->set_stream(particle_type_id, number);
    }
  }
  int particle_system::create_attractor()
  {
    particle_attractor* p_a = new particle_attractor();
    p_a->initialize();

    attractor_max_id++;
    id_to_attractor.insert(std::pair<int,particle_attractor*>(attractor_max_id, p_a));

    return attractor_max_id;
  }
  int particle_system::create_destroyer()
  {
    particle_destroyer* p_ds = new particle_destroyer();
    p_ds->initialize();

    destroyer_max_id++;
    id_to_destroyer.insert(std::pair<int,particle_destroyer*>(destroyer_max_id, p_ds));

    return destroyer_max_id;
  }
  int particle_system::create_deflector()
  {
    particle_deflector* p_df = new particle_deflector();
    p_df->initialize();

    deflector_max_id++;
    id_to_deflector.insert(std::pair<int,particle_deflector*>(deflector_max_id, p_df));

    return deflector_max_id;
  }
  int particle_system::create_changer()
  {
    particle_changer* p_c = new particle_changer();
    p_c->initialize();

    changer_max_id++;
    id_to_changer.insert(std::pair<int,particle_changer*>(changer_max_id, p_c));

    return changer_max_id;
  }
}

namespace enigma
{
  struct particle_system_manager
  {
    int max_id;
    std::map<int,particle_system*> id_to_particlesystem;
  };

  particle_system_manager ps_manager;

  void update_particlesystems()
  {
    std::map<int,particle_system*>::iterator end = ps_manager.id_to_particlesystem.end();
    for (std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.begin(); it != end; it++)
    {
      if ((*it).second->auto_update) {
        (*it).second->update_particlesystem();
      }
    }
  }

  void draw_particlesystems(std::set<int>& particlesystem_ids)
  {
    std::set<int>::iterator end = particlesystem_ids.end();
    for (std::set<int>::iterator it = particlesystem_ids.begin(); it != end; it++)
    {
      std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(*it);
      if (ps_it != ps_manager.id_to_particlesystem.end()) {
        if ((*ps_it).second->auto_draw) {
          (*ps_it).second->draw_particlesystem();
        }
      }
    }
  }
}

using enigma::particle_system;
using enigma::particle_type;
using enigma::ps_manager;
using enigma::ps_manager;
using enigma::particle_type_manager;
using enigma::particle_emitter;
using enigma::particle_attractor;
using enigma::particle_destroyer;
using enigma::particle_deflector;
using enigma::particle_changer;

// General functions.

int part_system_create()
{
  particle_system* p_s = new particle_system();
  p_s->initialize();

  ps_manager.max_id++;
  ps_manager.id_to_particlesystem.insert(std::pair<int,particle_system*>(ps_manager.max_id, p_s));
  p_s->id = ps_manager.max_id;

  // Drawing is automatic, so register in depth.
  enigma::drawing_depths[p_s->depth].particlesystem_ids.insert(ps_manager.max_id);

  return ps_manager.max_id;
}
void part_system_destroy(int id)
{
  part_system_clear(id);
  // Remember to destroy the system.
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*it).second;
    delete p_s;
    ps_manager.id_to_particlesystem.erase(it);
  }
}
bool part_system_exists(int id)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    return true;
  }
  return false;
}
void part_system_clear(int id)
{
  part_emitter_destroy_all(id);
  part_attractor_destroy_all(id);
  part_destroyer_destroy_all(id);
  part_deflector_destroy_all(id);
  part_changer_destroy_all(id);
  part_particles_clear(id);
}
void part_system_draw_order(int id, bool oldtonew)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    (*it).second->oldtonew = oldtonew;
  }
}
void part_system_depth(int id, double depth)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*it).second;
    const double new_depth = round(depth);
    if (p_s->auto_draw) {
      // If the particle system has automatic drawing enabled, it is in the depth system,
      // and it should be moved.
      const double current_depth = p_s->depth;
      enigma::drawing_depths[current_depth].particlesystem_ids.erase(p_s->id);
      enigma::drawing_depths[new_depth].particlesystem_ids.insert(p_s->id);
    }
    else {
      // If the particle system does not have automatic drawing enabled, it is not in the depth system,
      // and the value should just be updated.
      p_s->depth = new_depth;
    }
  }
}
void part_system_position(int id, double x, double y)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    (*it).second->x_offset = x;
    (*it).second->y_offset = y;
  }
}

// Update and draw.

void part_system_automatic_update(int id, bool automatic)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    (*it).second->auto_update = automatic;
  }
}
void part_system_automatic_draw(int id, bool automatic)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*it).second;
    bool auto_draw_before = p_s->auto_draw;
    p_s->auto_draw = automatic;
    if (automatic && !auto_draw_before) { // Add to drawing depths.
      enigma::drawing_depths[p_s->depth].particlesystem_ids.insert(p_s->id);
    }
    else if (!automatic && auto_draw_before) { // Remove from drawing depths.
      enigma::drawing_depths[p_s->depth].particlesystem_ids.erase(p_s->id);
    }
  }
}
void part_system_update(int id)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    (*it).second->update_particlesystem();
  }
}
void part_system_drawit(int id)
{
  std::map<int,particle_system*>::iterator it = ps_manager.id_to_particlesystem.find(id);
  if (it != ps_manager.id_to_particlesystem.end()) {
    (*it).second->draw_particlesystem();
  }
}

// Particles.

void part_particles_create(int id, double x, double y, int particle_type_id, int number)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    std::map<int,particle_type*>::iterator pt_it = pt_manager.id_to_particletype.find(id);
    if (pt_it != pt_manager.id_to_particletype.end()) {
      (*ps_it).second->create_particles(x, y, (*pt_it).second, number);
    }
  }
}
void part_particles_create_color(int id, double x, double y, int particle_type_id, int color, int number)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    std::map<int,particle_type*>::iterator pt_it = pt_manager.id_to_particletype.find(id);
    if (pt_it != pt_manager.id_to_particletype.end()) {
      (*ps_it).second->create_particles(x, y, (*pt_it).second, number, true, color);
    }
  }
}
void part_particles_clear(int id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    for (std::list<enigma::particle_instance>::iterator it = p_s->pi_list.begin(); it != p_s->pi_list.end(); it++)
    {
      particle_type* pt = it->pt;

      // Death handling.
      pt->particle_count--;
      if (pt->particle_count <= 0 && !pt->alive) {
        // Particle type is no longer used, delete it.
        int id = pt->id;
        delete pt;
        enigma::pt_manager.id_to_particletype.erase(id);
      }
    }
    p_s->pi_list.clear();
  }
}
void part_particles_position(int id, double x, double y)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    p_s->x_offset = x;
    p_s->y_offset = y;
  }
}

// Emitters.

int part_emitter_create(int id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    return (*ps_it).second->create_emitter();
  }
  return -1;
}
void part_emitter_destroy(int ps_id, int em_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_emitter*>::iterator em_it = p_s->id_to_emitter.find(em_id);
    if (em_it != p_s->id_to_emitter.end()) {
      delete (*em_it).second;
      p_s->id_to_emitter.erase(em_it);
    }
  }
}
void part_emitter_destroy_all(int ps_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    for (std::map<int,particle_emitter*>::iterator it = p_s->id_to_emitter.begin(); it != p_s->id_to_emitter.end(); it++)
    {
      delete (*it).second;
    }
    p_s->id_to_emitter.clear();
  }
}
bool part_emitter_exists(int ps_id, int em_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_emitter*>::iterator em_it = p_s->id_to_emitter.find(em_id);
    if (em_it != p_s->id_to_emitter.end()) {
      return true;
    }
  }
  return false;
}
void part_emitter_clear(int ps_id, int em_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_emitter*>::iterator em_it = p_s->id_to_emitter.find(em_id);
    if (em_it != p_s->id_to_emitter.end()) {
      (*em_it).second->initialize();
    }
  }
}
void part_emitter_region(int ps_id, int em_id, double xmin, double xmax, double ymin, double ymax, int shape, int distribution)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    (*ps_it).second->set_emitter_region(em_id, xmin, xmax, ymin, ymax, enigma::get_ps_shape(shape), enigma::get_ps_distr(distribution));
  }
}
void part_emitter_burst(int ps_id, int em_id, int particle_type_id, int number)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_emitter*>::iterator em_it = p_s->id_to_emitter.find(em_id);
    if (em_it != p_s->id_to_emitter.end()) {
      particle_emitter* p_e = (*em_it).second;
      std::map<int,particle_type*>::iterator pt_it = pt_manager.id_to_particletype.find(particle_type_id);
      if (pt_it != pt_manager.id_to_particletype.end()) {
        particle_type* p_t = (*pt_it).second;
        for (int i = 1; i <= number; i++)
        {
          int x, y;
          p_e->get_point(x, y);
          p_s->create_particles(x, y, p_t, 1);
        }
      }
    }
  }
}
void part_emitter_stream(int ps_id, int em_id, int particle_type_id, int number)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    (*ps_it).second->set_emitter_stream(em_id, particle_type_id, number);
  }
}

// Attractor.

int part_attractor_create(int id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    return (*ps_it).second->create_attractor();
  }
  return -1;
}
void part_attractor_destroy(int ps_id, int at_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_attractor*>::iterator at_it = p_s->id_to_attractor.find(at_id);
    if (at_it != p_s->id_to_attractor.end()) {
      delete (*at_it).second;
      p_s->id_to_attractor.erase(at_it);
    }
  }
}
void part_attractor_destroy_all(int ps_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    for (std::map<int,particle_attractor*>::iterator it = p_s->id_to_attractor.begin(); it != p_s->id_to_attractor.end(); it++)
    {
      delete (*it).second;
    }
    p_s->id_to_attractor.clear();
  }
}
bool part_attractor_exists(int ps_id, int at_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_attractor*>::iterator at_it = p_s->id_to_attractor.find(at_id);
    if (at_it != p_s->id_to_attractor.end()) {
      return true;
    }
  }
  return false;
}
void part_attractor_clear(int ps_id, int at_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_attractor*>::iterator at_it = p_s->id_to_attractor.find(at_id);
    if (at_it != p_s->id_to_attractor.end()) {
      (*at_it).second->initialize();
    }
  }
}
void part_attractor_position(int ps_id, int at_id, double x, double y)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_attractor*>::iterator at_it = p_s->id_to_attractor.find(at_id);
    if (at_it != p_s->id_to_attractor.end()) {
      (*at_it).second->set_position(x, y);
    }
  }
}
void part_attractor_force(int ps_id, int at_id, double force, double dist, int kind, bool additive)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_attractor*>::iterator at_it = p_s->id_to_attractor.find(at_id);
    if (at_it != p_s->id_to_attractor.end()) {
      (*at_it).second->set_force(force, dist, enigma::get_ps_force(kind), additive);
    }
  }
}

// Destroyer.

int part_destroyer_create(int id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    return (*ps_it).second->create_destroyer();
  }
  return -1;
}
void part_destroyer_destroy(int ps_id, int ds_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_destroyer*>::iterator ds_it = p_s->id_to_destroyer.find(ds_id);
    if (ds_it != p_s->id_to_destroyer.end()) {
      delete (*ds_it).second;
      p_s->id_to_destroyer.erase(ds_it);
    }
  }
}
void part_destroyer_destroy_all(int ps_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    for (std::map<int,particle_destroyer*>::iterator it = p_s->id_to_destroyer.begin(); it != p_s->id_to_destroyer.end(); it++)
    {
      delete (*it).second;
    }
    p_s->id_to_destroyer.clear();
  }
}
bool part_destroyer_exists(int ps_id, int ds_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_destroyer*>::iterator ds_it = p_s->id_to_destroyer.find(ds_id);
    if (ds_it != p_s->id_to_destroyer.end()) {
      return true;
    }
  }
  return false;
}
void part_destroyer_clear(int ps_id, int ds_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_destroyer*>::iterator ds_it = p_s->id_to_destroyer.find(ds_id);
    if (ds_it != p_s->id_to_destroyer.end()) {
      (*ds_it).second->initialize();
    }
  }
}
void part_destroyer_region(int ps_id, int ds_id, double xmin, double xmax, double ymin, double ymax, int shape)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_destroyer*>::iterator ds_it = p_s->id_to_destroyer.find(ds_id);
    if (ds_it != p_s->id_to_destroyer.end()) {
      (*ds_it).second->set_region(xmin, xmax, ymin, ymax, enigma::get_ps_shape(shape));
    }
  }
}

// Deflector.

int part_deflector_create(int id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    return (*ps_it).second->create_deflector();
  }
  return -1;
}
void part_deflector_destroy(int ps_id, int df_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_deflector*>::iterator df_it = p_s->id_to_deflector.find(df_id);
    if (df_it != p_s->id_to_deflector.end()) {
      delete (*df_it).second;
      p_s->id_to_deflector.erase(df_it);
    }
  }
}
void part_deflector_destroy_all(int ps_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    for (std::map<int,particle_deflector*>::iterator it = p_s->id_to_deflector.begin(); it != p_s->id_to_deflector.end(); it++)
    {
      delete (*it).second;
    }
    p_s->id_to_deflector.clear();
  }
}
bool part_deflector_exists(int ps_id, int df_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_deflector*>::iterator df_it = p_s->id_to_deflector.find(df_id);
    if (df_it != p_s->id_to_deflector.end()) {
      return true;
    }
  }
  return false;
}
void part_deflector_clear(int ps_id, int df_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_deflector*>::iterator df_it = p_s->id_to_deflector.find(df_id);
    if (df_it != p_s->id_to_deflector.end()) {
      (*df_it).second->initialize();
    }
  }
}
void part_deflector_region(int ps_id, int df_id, double xmin, double xmax, double ymin, double ymax)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_deflector*>::iterator df_it = p_s->id_to_deflector.find(df_id);
    if (df_it != p_s->id_to_deflector.end()) {
      (*df_it).second->set_region(xmin, xmax, ymin, ymax);
    }
  }
}
void part_deflector_kind(int ps_id, int df_id, int kind)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_deflector*>::iterator df_it = p_s->id_to_deflector.find(df_id);
    if (df_it != p_s->id_to_deflector.end()) {
      (*df_it).second->set_kind(enigma::get_ps_deflect(kind));
    }
  }
}
void part_deflector_friction(int ps_id, int df_id, double friction)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_deflector*>::iterator df_it = p_s->id_to_deflector.find(df_id);
    if (df_it != p_s->id_to_deflector.end()) {
      (*df_it).second->set_friction(friction);
    }
  }
}

// Changer.

int part_changer_create(int id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    return (*ps_it).second->create_changer();
  }
  return -1;
}
void part_changer_destroy(int ps_id, int ch_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_changer*>::iterator ch_it = p_s->id_to_changer.find(ch_id);
    if (ch_it != p_s->id_to_changer.end()) {
      delete (*ch_it).second;
      p_s->id_to_changer.erase(ch_it);
    }
  }
}
void part_changer_destroy_all(int ps_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    for (std::map<int,particle_changer*>::iterator it = p_s->id_to_changer.begin(); it != p_s->id_to_changer.end(); it++)
    {
      delete (*it).second;
    }
    p_s->id_to_changer.clear();
  }
}
bool part_changer_exists(int ps_id, int ch_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_changer*>::iterator ch_it = p_s->id_to_changer.find(ch_id);
    if (ch_it != p_s->id_to_changer.end()) {
      return true;
    }
  }
  return false;
}
void part_changer_clear(int ps_id, int ch_id)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_changer*>::iterator ch_it = p_s->id_to_changer.find(ch_id);
    if (ch_it != p_s->id_to_changer.end()) {
      (*ch_it).second->initialize();
    }
  }
}
void part_changer_region(int ps_id, int ch_id, double xmin, double xmax, double ymin, double ymax, int shape)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_changer*>::iterator ch_it = p_s->id_to_changer.find(ch_id);
    if (ch_it != p_s->id_to_changer.end()) {
      (*ch_it).second->set_region(xmin, xmax, ymin, ymax, enigma::get_ps_shape(shape));
    }
  }
}
void part_changer_types(int ps_id, int ch_id, int parttype1, int parttype2)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_changer*>::iterator ch_it = p_s->id_to_changer.find(ch_id);
    if (ch_it != p_s->id_to_changer.end()) {
      (*ch_it).second->set_types(parttype1, parttype2);
    }
  }
}
void part_changer_kind(int ps_id, int ch_id, int kind)
{
  std::map<int,particle_system*>::iterator ps_it = ps_manager.id_to_particlesystem.find(ps_id);
  if (ps_it != ps_manager.id_to_particlesystem.end()) {
    particle_system* p_s = (*ps_it).second;
    std::map<int,particle_changer*>::iterator ch_it = p_s->id_to_changer.find(ch_id);
    if (ch_it != p_s->id_to_changer.end()) {
      (*ch_it).second->set_change_kind(enigma::get_ps_change(kind));
    }
  }
}

