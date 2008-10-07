/* 
 * libnova C++ extension.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __LIBNOVA_CPP__
#define __LIBNOVA_CPP__

#include <libnova/libnova.h>
#include <ostream>
#include <time.h>
#include <math.h>

/**
 * @file Libnova utility classes
 *
 * @defgroup LibnovaCPP Libnova clases
 */

/**
 * Returns time from JD in number of seconds from 1-1-1970.
 */
double timetFromJD (double JD);

/**
 * Holds RA of an target.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaRa
{
	protected:
		double ra;
		void toHms (struct ln_hms *ra_hms);
		void fromHms (struct ln_hms *ra_hms);
	public:
		LibnovaRa ()
		{
			ra = nan ("f");
		}
		LibnovaRa (double in_ra)
		{
			ra = in_ra;
		}
		double getRa ()
		{
			return ra;
		}

		/**
		 * Flip by 12 hours.
		 */
		void flip ();

		friend std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
		friend std::istream & operator >> (std::istream & _is, LibnovaRa & l_ra);
};

/**
 * Holds target entered in J2000 coordinate system.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaRaJ2000:public LibnovaRa
{
	public:
		LibnovaRaJ2000 ():LibnovaRa ()
		{
		}
		LibnovaRaJ2000 (double in_ra):LibnovaRa (in_ra)
		{
		}
		friend std::ostream & operator << (std::ostream & _os, LibnovaRaJ2000 l_ra);
};

/**
 * Holds hours and minutes.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaHaM:public LibnovaRa
{
	public:
		LibnovaHaM ():LibnovaRa ()
		{
		}
		LibnovaHaM (double in_ha):LibnovaRa (in_ha)
		{
		}
		friend std::ostream & operator << (std::ostream & _os, LibnovaHaM l_haM);
		friend std::istream & operator >> (std::istream & _is, LibnovaHaM & l_haM);
};

/**
 * Distance in RA.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaRaComp:public LibnovaRa
{
	public:
		LibnovaRaComp ():LibnovaRa ()
		{
		}
		LibnovaRaComp (double in_ra):LibnovaRa (in_ra)
		{
		}
		friend std::ostream & operator << (std::ostream & _os, LibnovaRaComp l_ra);
};

/**
 * Abstract degrees class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDeg
{
	private:
		void fromNegDouble (bool neg, double res)
		{
			if (neg)
				res *= -1;
			deg = res;
		}
	protected:
		double deg;
		void toDms (struct ln_dms *deg_dms);
		void fromDms (struct ln_dms *deg_dms);
	public:
		LibnovaDeg ()
		{
			deg = nan ("f");
		}
		LibnovaDeg (double in_deg)
		{
			deg = in_deg;
		}
		LibnovaDeg (struct ln_dms *deg_dms)
		{
			fromDms (deg_dms);
		}
		double getDeg ()
		{
			return deg;
		}
		friend std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
		/**
		 * Input operator. Accept inputs in following forms:
		 *
		 * <ul>
		 *   <li>[+-]DDD.ddd</li>
		 *   <li>[+-]DDD:MM.mmm</li>
		 *   <li>[+-]DDD:MM:SS.ssss</li>
		 * </ul>
		 *
		 * It assumes that : or space is 60th unit delimiter, while next + or - mark another deg unit.
		 *
		 * So "+25:45:16" is one LibnovaDeg value, but "+25 45 16 +87 56 78" will be
		 * readed as two LibnovaDeg values.
		 */
		friend std::istream & operator >> (std::istream & _is, LibnovaDeg & l_deg);
};

/**
 * Deg -90..90 class, used tyo display DEC.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDeg90:public LibnovaDeg
{
	public:
		LibnovaDeg90 ():LibnovaDeg ()
		{
		}
		LibnovaDeg90 (double in_deg):LibnovaDeg (in_deg)
		{
		}

		friend std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
};

/**
 * Deg 0..360 class, used to display RA and HA in degrees.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDeg360:public LibnovaDeg
{
	public:
		LibnovaDeg360 ():LibnovaDeg ()
		{
		}
		LibnovaDeg360 (double in_deg):LibnovaDeg (ln_range_degrees (in_deg))
		{
		}

		friend std::ostream & operator << (std::ostream & _os, LibnovaDeg360 l_deg);
};

/**
 * Deg 0..180 class, used to display distance in degrees.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDeg180:public LibnovaDeg
{
	public:
		LibnovaDeg180 ():LibnovaDeg ()
		{
		}
		LibnovaDeg180 (double in_deg):LibnovaDeg (ln_range_degrees (in_deg))
		{
		}

		friend std::ostream & operator << (std::ostream & _os, LibnovaDeg180 l_deg);
};

/**
 * Dec class for DEC display.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDec:public LibnovaDeg90
{
	public:
		LibnovaDec ():LibnovaDeg90 ()
		{
		}
		LibnovaDec (double in_deg):LibnovaDeg90 (in_deg)
		{
		}
		double getDec ()
		{
			return getDeg ();
		}

		void flip (struct ln_lnlat_posn *obs);

		friend std::ostream & operator << (std::ostream & _os, LibnovaDec l_dec);
};

/**
 * Dec in J2000 coordinates.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDecJ2000:public LibnovaDec
{
	public:
		LibnovaDecJ2000 ():LibnovaDec ()
		{
		}
		LibnovaDecJ2000 (double in_deg):LibnovaDec (in_deg)
		{
		}

		friend std::ostream & operator << (std::ostream & _os,
			LibnovaDecJ2000 l_dec);
};

/**
 * 90 deg comparsion.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDeg90Comp:public LibnovaDeg90
{
	public:
		LibnovaDeg90Comp ():LibnovaDeg90 ()
		{
		}
		LibnovaDeg90Comp (double in_deg):LibnovaDeg90 (in_deg)
		{
		}

		friend std::ostream & operator << (std::ostream & _os,
			LibnovaDeg90Comp l_deg);
};

/**
 * Display degrees as arc min.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDegArcMin:public LibnovaDeg
{
	public:
		LibnovaDegArcMin ():LibnovaDeg ()
		{
		}
		LibnovaDegArcMin (double in_deg):LibnovaDeg (in_deg)
		{
		}

		friend std::ostream & operator << (std::ostream & _os,
			LibnovaDegArcMin l_deg);
};

/**
 * Display degrees as distance (scale between degrees, minutes and seconds dispaly).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDegDist:public LibnovaDeg
{
	public:
		LibnovaDegDist ():LibnovaDeg ()
		{
		}
		LibnovaDegDist (double in_deg):LibnovaDeg (in_deg)
		{
		}

		friend std::ostream & operator << (std::ostream & _os,
			LibnovaDegDist l_deg);
		friend std::istream & operator >> (std::istream & _is,
			LibnovaDegDist & l_deg);
};

/**
 * Holds RA and DEC of an object.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaRaDec
{
	private:
		LibnovaRa * ra;
		LibnovaDec *dec;
	public:
		LibnovaRaDec ()
		{
			ra = NULL;
			dec = NULL;
		}

		LibnovaRaDec (LibnovaRaDec &in_libnova)
		{
			ra = new LibnovaRa (in_libnova.getRa ());
			dec = new LibnovaDec (in_libnova.getDec ());
		}

		LibnovaRaDec (struct ln_equ_posn *pos)
		{
			ra = new LibnovaRa (pos->ra);
			dec = new LibnovaDec (pos->dec);
		}

		LibnovaRaDec (double in_ra, double in_dec)
		{
			ra = new LibnovaRa (in_ra);
			dec = new LibnovaDec (in_dec);
		}

		virtual ~ LibnovaRaDec (void)
		{
			delete ra;
			delete dec;
		}

		void setRa (double in_ra)
		{
			delete ra;
			ra = new LibnovaRa (in_ra);
		}

		double getRa ()
		{
			if (ra)
				return ra->getRa ();
			return nan ("f");
		}

		void setDec (double in_dec)
		{
			delete dec;
			dec = new LibnovaDec (in_dec);
		}

		double getDec ()
		{
			if (dec)
				return dec->getDec ();
			return nan ("f");
		}

		void getPos (struct ln_equ_posn *pos)
		{
			pos->ra = getRa ();
			pos->dec = getDec ();
		}

		void setPos (struct ln_equ_posn *pos)
		{
			delete ra;
			delete dec;
			ra = new LibnovaRa (pos->ra);
			dec = new LibnovaDec (pos->dec);
		}

		LibnovaRa *getRaObj ()
		{
			return ra;
		}

		LibnovaDec *getDecObj ()
		{
			return dec;
		}

		/**
		 * Flip current RA and DEC.
		 *
		 * @param obs Coordinates of the observing station.
		 */
		void flip (struct ln_lnlat_posn *obs);

		friend std::ostream & operator << (std::ostream & _os,
			LibnovaRaDec l_radec);
		friend std::istream & operator >> (std::istream & _is,
			LibnovaRaDec & l_radec);

		/**
		 * Get Ra and Dec from string.
		 */
		int parseString (const char *radec);
};

/**
 * Holds horizontal coordinates - altitude and azimuth - of an object.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaHrz
{
	private:
		LibnovaDeg90 * alt;
		LibnovaDeg360 *az;
	public:
		LibnovaHrz ()
		{
			alt = NULL;
			az = NULL;
		}

		LibnovaHrz (LibnovaHrz & in_libnova)
		{
			alt = new LibnovaDeg90 (in_libnova.getAlt ());
			az = new LibnovaDeg360 (in_libnova.getAz ());
		}

		LibnovaHrz (struct ln_hrz_posn *hrz)
		{
			alt = new LibnovaDeg90 (hrz->alt);
			az = new LibnovaDeg360 (hrz->az);
		}

		LibnovaHrz (double in_alt, double in_az)
		{
			alt = new LibnovaDeg90 (in_alt);
			az = new LibnovaDeg360 (in_az);
		}

		virtual ~ LibnovaHrz (void)
		{
			delete alt;
			delete az;
		}

		double getAlt ()
		{
			if (alt)
				return alt->getDeg ();
			return nan ("f");
		}

		double getAz ()
		{
			if (az)
				return az->getDeg ();
			return nan ("f");
		}

		void getHrz (struct ln_hrz_posn *hrz)
		{
			hrz->alt = getAlt ();
			hrz->az = getAz ();
		}

		void setHrz (struct ln_hrz_posn *hrz)
		{
			delete alt;
			delete az;
			alt = new LibnovaDeg90 (hrz->alt);
			az = new LibnovaDeg360 (hrz->az);
		}

		friend std::ostream & operator << (std::ostream & _os, LibnovaHrz l_hrz);
};

/**
 * Holds date.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDate
{
	protected:
		struct ln_date date;
		// don't intialize value
		LibnovaDate (bool sysinit)
		{
		}
	public:
		LibnovaDate ()
		{
			ln_get_date_from_sys (&date);
		}

		LibnovaDate (double JD)
		{
			ln_get_date (JD, &date);
		}

		LibnovaDate (time_t * t)
		{
			ln_get_date_from_timet (t, &date);
		}

		LibnovaDate (struct ln_date *_date)
		{
			date.years = _date->years + 1900;
			date.months = _date->months + 1;
			date.days = _date->days;
			date.hours = _date->hours;
			date.minutes = _date->minutes;
			date.seconds = _date->seconds;
		}

		/**
		 * Construct date from system struct tm.
		 */
		LibnovaDate (struct tm *_date)
		{
			date.years = _date->tm_year + 1900;
			date.months = _date->tm_mon + 1;
			date.days = _date->tm_mday;
			date.hours = _date->tm_hour;
			date.minutes = _date->tm_min;
			date.seconds = _date->tm_sec;
		}

		void getTimeT (time_t * t)
		{
			ln_get_timet_from_julian (ln_get_julian_day (&date), t);
		}

		friend std::ostream & operator << (std::ostream & _os, LibnovaDate l_date);
		friend std::istream & operator >> (std::istream & _is,
			LibnovaDate & l_date);
};

/**
 * Holds date, which can be created from double in computer time.
 * Create LibnovaData class from seconds and fractions of seconds
 * from 1-1-1970.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaDateDouble:public LibnovaDate
{
	public:
		/**
		 * Create LibnovaDate from seconds and fractions of
		 * seconds.
		 *
		 * @param t Seconds and fractions of seconds from 1-1-1970 (computer time).
		 */
		LibnovaDateDouble (double t):LibnovaDate (false)
		{
			time_t tt = (time_t) ceil (t);
			ln_get_date_from_timet (&tt, &date);
			date.seconds += (t - tt);
		}
};

/**
 * Calculate from date night - e.g. date/time at which night started and at which it ends
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class Rts2Night
{
	private:
		time_t from;
		time_t to;
	public:
		Rts2Night ()
		{
			time (&to);
			from = to - 86400;
		}
		Rts2Night (struct ln_date *ln_night, struct ln_lnlat_posn *obs);
		Rts2Night (double JD, struct ln_lnlat_posn *obs);

		time_t *getFrom ()
		{
			return &from;
		}
		time_t *getTo ()
		{
			return &to;
		}

		friend std::ostream & operator << (std::ostream & _os, Rts2Night night);
};

/**
 * Holds observer position on Earth (longitude and latitude).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @ingroup LibnovaCPP
 */
class LibnovaPos
{
	private:
		struct ln_lnlat_posn pos;
	public:
		LibnovaPos (const struct ln_lnlat_posn *in_pos)
		{
			pos.lng = in_pos->lng;
			pos.lat = in_pos->lat;
		}
		LibnovaPos (double in_lng, double in_lat)
		{
			pos.lng = in_lng;
			pos.lat = in_lat;
		}

		double getLongitude ()
		{
			return pos.lng;
		}
		double getLatitude ()
		{
			return pos.lat;
		}

		friend std::ostream & operator << (std::ostream & _os, LibnovaPos l_pos);
};

std::ostream & operator << (std::ostream & _os, LibnovaRa l_ra);
std::istream & operator >> (std::istream & _os, LibnovaRa & l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaRaJ2000 l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaHaM l_haM);
std::istream & operator >> (std::istream & _is, LibnovaHaM & l_haM);
std::ostream & operator << (std::ostream & _os, LibnovaRaComp l_ra);
std::ostream & operator << (std::ostream & _os, LibnovaDeg l_deg);
std::istream & operator >> (std::istream & _is, LibnovaDeg & l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90 l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDeg360 l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDec l_dec);
std::ostream & operator << (std::ostream & _os, LibnovaDecJ2000 l_dec);
std::ostream & operator << (std::ostream & _os, LibnovaDeg90Comp l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegArcMin l_deg);
std::ostream & operator << (std::ostream & _os, LibnovaDegDist l_deg);
std::istream & operator >> (std::istream & _is, LibnovaDegDist & l_deg);

std::ostream & operator << (std::ostream & _os, LibnovaRaDec l_radec);
std::istream & operator >> (std::istream & _is, LibnovaRaDec & l_radec);

std::ostream & operator << (std::ostream & _os, LibnovaHrz l_hrz);

std::ostream & operator << (std::ostream & _os, LibnovaDate l_date);
std::istream & operator >> (std::istream & _is, LibnovaDate & l_date);
std::ostream & operator << (std::ostream & _os, Rts2Night night);

std::ostream & operator << (std::ostream & _os, LibnovaPos l_pos);
#endif							 /* !__LIBNOVA_CPP__ */
