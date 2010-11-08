/*
 * Connections for image processing forked instances.
 * Copyright (C) 2003-2009 Petr Kubanek <petr@kubanek.net>
 * Copyright (C) 2010 Petr Kubanek, Institute of Physics
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

#ifndef __RTS2CONNIMGPROCESS__
#define __RTS2CONNIMGPROCESS__

#include "connexe.h"
#include "../writers/rts2imagedb.h"
#include "../utilsdb/observation.h"

namespace rts2plan
{

typedef enum { NOT_ASTROMETRY, TRASH, GET, DARK, BAD, FLAT } astrometry_stat_t;

class ConnProcess:public rts2script::ConnExe
{
	public:
		ConnProcess (Rts2Block * in_master, const char *in_exe, int in_timeout);

		astrometry_stat_t getAstrometryStat () { return astrometryStat; }
		
		double getExposureEnd () { return expDate; };

#ifdef HAVE_LIBJPEG
		void setLastGoodJpeg (const char *_last_good_jpeg) { last_good_jpeg = _last_good_jpeg; }
		void setLastTrashJpeg (const char *_last_trash_jpeg) { last_trash_jpeg = _last_trash_jpeg; }
#endif

	protected:
		astrometry_stat_t astrometryStat;
		double expDate;

#ifdef HAVE_LIBJPEG
		const char *last_good_jpeg;
		const char *last_trash_jpeg;
#endif
};

/**
 * "Connection" which reads output of image processor
 *
 * This class expect that images are stored in CENTRAL repository,
 * accesible throught NFS/other network sharing to all machines on
 * which imgproc runs.
 *
 * Hence passing full image path will be sufficient for finding
 * it.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnImgProcess:public ConnProcess
{
	public:
		/**
		 *
		 * @param _end_event  If set to value > 0, this event will be emmited at the end of image processing, with image passed
		 *	as argument. Then the standard image processing - bad to trash, with astrometry to archive - will not be run.
		 */
		ConnImgProcess (Rts2Block *_master, const char *_exe, const char *_path, int _timeout, int _end_event = -1);
		virtual ~ ConnImgProcess (void);

		virtual int init ();

		virtual int newProcess ();
		virtual void processLine ();

	protected:
		virtual void processCommand (char *cmd);
	
		virtual void connectionError (int last_data_size);

	private:
		std::string imgPath;

		long id;
		double ra, dec, ra_err, dec_err;

		int end_event;
};

class ConnObsProcess:public ConnProcess
{
	private:
		int obsId;
		rts2db::Observation *obs;

		char *obsIdCh;
		char *obsTarIdCh;
		char *obsTarTypeCh;
	public:
		ConnObsProcess (Rts2Block * in_master,
			const char *in_exe, int in_obsId, int in_timeout);

		virtual int newProcess ();
		virtual void processLine ();
};

class ConnDarkProcess:public ConnProcess
{
	public:
		ConnDarkProcess (Rts2Block * in_master,
			const char *in_exe, int in_timeout);

		virtual void processLine ();
};

class ConnFlatProcess:public ConnProcess
{
	public:
		ConnFlatProcess (Rts2Block * in_master,
			const char *in_exe, int in_timeout);

		virtual void processLine ();
};

};
#endif							 /* !__RTS2CONNIMGPROCESS__ */
