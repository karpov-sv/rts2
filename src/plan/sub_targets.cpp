#include <libnova/libnova.h>
#include "target.h"

// ConstTarget
ConstTarget::ConstTarget (struct device * tel, struct ln_lnlat_posn * obs, struct ln_equ_posn * in_pos):
Target (tel, obs)
{
  position = *in_pos;
}

int
ConstTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  *pos = position;
  return 0;
}

int
ConstTarget::getRST (struct ln_lnlat_posn *observer,
		     struct ln_rst_time *rst, double JD)
{
  return ln_get_object_rst (JD, observer, &position, rst);
}

// EllTarget - good for commets and so on
EllTarget::EllTarget (struct device * tel, struct ln_lnlat_posn * obs, struct ln_ell_orbit * in_orbit):Target (tel,
	obs)
{
  orbit = *in_orbit;
}

int
EllTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  if (orbit.e == 1.0)
    {
      struct ln_par_orbit par_orbit;
      par_orbit.q = orbit.a;
      par_orbit.i = orbit.i;
      par_orbit.w = orbit.w;
      par_orbit.omega = orbit.omega;
      par_orbit.JD = orbit.JD;
      ln_get_par_body_equ_coords (JD, &par_orbit, pos);
      return 0;
    }
  else if (orbit.e > 1.0)
    {
      struct ln_hyp_orbit hyp_orbit;
      hyp_orbit.q = orbit.a;
      hyp_orbit.e = orbit.e;
      hyp_orbit.i = orbit.i;
      hyp_orbit.w = orbit.w;
      hyp_orbit.omega = orbit.omega;
      hyp_orbit.JD = orbit.JD;
      ln_get_hyp_body_equ_coords (JD, &hyp_orbit, pos);
      return 0;
    }
  ln_get_ell_body_equ_coords (JD, &orbit, pos);
  return 0;
}

int
EllTarget::getRST (struct ln_lnlat_posn *observer,
		   struct ln_rst_time *rst, double JD)
{
  return ln_get_ell_body_rst (JD, observer, &orbit, rst);
}

// Parabolic target - also for commets
ParTarget::ParTarget (struct device * tel, struct ln_lnlat_posn * obs, struct ln_par_orbit * in_orbit):Target (tel,
	obs)
{
  orbit = *in_orbit;
}

int
ParTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  ln_get_par_body_equ_coords (JD, &orbit, pos);
  return 0;
}

int
ParTarget::getRST (struct ln_lnlat_posn *observer,
		   struct ln_rst_time *rst, double JD)
{
  return ln_get_par_body_rst (JD, observer, &orbit, rst);
}

// will pickup the Moon
LunarTarget::LunarTarget (struct device * tel, struct ln_lnlat_posn * obs):Target (tel,
	obs)
{
}

int
LunarTarget::getPosition (struct ln_equ_posn *pos, double JD)
{
  ln_get_lunar_equ_coords (JD, pos, 0);
  return 0;
}

int
LunarTarget::getRST (struct ln_lnlat_posn *observer,
		     struct ln_rst_time *rst, double JD)
{
  return ln_get_lunar_rst (JD, observer, rst);
}
