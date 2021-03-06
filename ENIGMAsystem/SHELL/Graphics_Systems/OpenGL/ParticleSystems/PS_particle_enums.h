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

#ifndef ENIGMA_PS_PARTICLEENUMS
#define ENIGMA_PS_PARTICLEENUMS

#include "PS_particle_constants.h"

namespace enigma
{
  enum pt_shape {
    pt_sh_pixel = 0,
    pt_sh_disk,
    pt_sh_square,
    pt_sh_line,
    pt_sh_star,
    pt_sh_circle,
    pt_sh_ring,
    pt_sh_sphere,
    pt_sh_flare,
    pt_sh_spark,
    pt_sh_explosion,
    pt_sh_cloud,
    pt_sh_smoke,
    pt_sh_snow
  };
  inline pt_shape get_pt_shape(int particle_shape)
  {
    switch (particle_shape) {
    case pt_shape_pixel : return enigma::pt_sh_pixel;
    case pt_shape_disk : return enigma::pt_sh_disk;
    case pt_shape_square : return enigma::pt_sh_square;
    case pt_shape_line : return enigma::pt_sh_line;
    case pt_shape_star : return enigma::pt_sh_star;
    case pt_shape_circle : return enigma::pt_sh_circle;
    case pt_shape_ring : return enigma::pt_sh_ring;
    case pt_shape_sphere : return enigma::pt_sh_sphere;
    case pt_shape_flare : return enigma::pt_sh_flare;
    case pt_shape_spark : return enigma::pt_sh_spark;
    case pt_shape_explosion : return enigma::pt_sh_explosion;
    case pt_shape_cloud : return enigma::pt_sh_cloud;
    case pt_shape_smoke : return enigma::pt_sh_smoke;
    case pt_shape_snow : return enigma::pt_sh_snow;
    default : return enigma::pt_sh_pixel;
    }
  }

  enum ps_shape {
    ps_sh_rectangle = 0,
    ps_sh_ellipse,
    ps_sh_diamond,
    ps_sh_line
  };
  inline ps_shape get_ps_shape(int shape)
  {
    switch (shape) {
    case ps_shape_rectangle : return enigma::ps_sh_rectangle;
    case ps_shape_ellipse : return enigma::ps_sh_ellipse;
    case ps_shape_diamond : return enigma::ps_sh_diamond;
    case ps_shape_line : return enigma::ps_sh_line;
    default : return enigma::ps_sh_rectangle;
    }
  }

  enum ps_distr {
    ps_di_linear = 0,
    ps_di_gaussian,
    ps_di_invgaussian
  };
  inline ps_distr get_ps_distr(int distribution)
  {
    switch (distribution) {
    case ps_distr_linear : return enigma::ps_di_linear;
    case ps_distr_gaussian : return enigma::ps_di_gaussian;
    case ps_distr_invgaussian : return enigma::ps_di_invgaussian;
    default : return enigma::ps_di_linear;
    }
  }

  enum ps_force {
    ps_fo_constant = 0,
    ps_fo_linear,
    ps_fo_quadratic
  };
  inline ps_force get_ps_force(int force_kind)
  {
    switch (force_kind) {
    case ps_force_constant : return enigma::ps_fo_constant;
    case ps_force_linear : return enigma::ps_fo_linear;
    case ps_force_quadratic : return enigma::ps_fo_quadratic;
    default : return enigma::ps_fo_constant;
    }
  }

  enum ps_deflect {
    ps_de_horizontal = 0,
    ps_de_vertical
  };
  inline ps_deflect get_ps_deflect(int deflection_kind)
  {
    switch (deflection_kind) {
    case ps_deflect_horizontal : return enigma::ps_de_horizontal;
    case ps_deflect_vertical : return enigma::ps_de_vertical;
    default : return enigma::ps_de_horizontal;
    }
  }

  enum ps_change {
    ps_ch_all = 0
  };
  inline ps_change get_ps_change(int change_kind)
  {
    switch (change_kind) {
    case ps_change_all : return enigma::ps_ch_all;
    default : return enigma::ps_ch_all;
    }
  }
}

#endif // ENIGMA_PS_PARTICLEENUMS

