#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>

#include "rts2block.h"
#include "rts2devclient.h"

Rts2DevClient::Rts2DevClient (Rts2Conn * in_connection):Rts2Object ()
{
  connection = in_connection;
  processedBaseInfo = NOT_PROCESED;
  addValue (new Rts2ValueString ("type"));
  addValue (new Rts2ValueString ("serial"));
}

void
Rts2DevClient::addValue (Rts2Value * value)
{
  values.push_back (value);
}

Rts2Value *
Rts2DevClient::getValue (char *value_name)
{
  std::vector < Rts2Value * >::iterator val_iter;
  for (val_iter = values.begin (); val_iter != values.end (); val_iter++)
    {
      Rts2Value *val;
      val = (*val_iter);
      if (val->isValue (value_name))
	return val;
    }
  return NULL;
}

char *
Rts2DevClient::getValueChar (char *value_name)
{
  Rts2Value *val;
  val = getValue (value_name);
  if (val)
    return val->getValue ();
  return NULL;
}

double
Rts2DevClient::getValueDouble (char *value_name)
{
  Rts2Value *val;
  val = getValue (value_name);
  if (val)
    return val->getValueDouble ();
  return nan ("f");
}

int
Rts2DevClient::getValueInteger (char *value_name)
{
  Rts2Value *val;
  val = getValue (value_name);
  if (val)
    return val->getValueInteger ();
  return -1;
}

int
Rts2DevClient::command ()
{
  if (connection->isCommand ("V"))
    {
      char *name;
      if (connection->paramNextString (&name))
	return -1;
      std::vector < Rts2Value * >::iterator val_iter;
      for (val_iter = values.begin (); val_iter != values.end (); val_iter++)
	{
	  Rts2Value *value;
	  value = (*val_iter);
	  if (!strcmp (value->getName (), name))
	    return value->setValue (connection);
	}
    }
  return -2;
}

void
Rts2DevClient::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("priority"))
    {
      switch (state->value)
	{
	case 1:
	  getPriority ();
	  break;
	case 0:
	  lostPriority ();
	  break;
	}
    }
}

const char *
Rts2DevClient::getName ()
{
  return connection->getName ();
}

void
Rts2DevClient::getPriority ()
{
}

void
Rts2DevClient::lostPriority ()
{
}

void
Rts2DevClient::died ()
{
  lostPriority ();
}

Rts2DevClientCamera::Rts2DevClientCamera (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("chips"));
  addValue (new Rts2ValueInteger ("can_df"));
  addValue (new Rts2ValueInteger ("temperature_regulation"));
  addValue (new Rts2ValueDouble ("temperature_setpoint"));
  addValue (new Rts2ValueDouble ("air_temperature"));
  addValue (new Rts2ValueDouble ("ccd_temperature"));
  addValue (new Rts2ValueInteger ("cooling_power"));
  addValue (new Rts2ValueInteger ("fan"));
  addValue (new Rts2ValueInteger ("filter"));
  addValue (new Rts2ValueString ("focuser"));

  addValue (new Rts2ValueDouble ("exposure"));
}

void
Rts2DevClientCamera::exposureStarted ()
{
}

void
Rts2DevClientCamera::exposureEnd ()
{
}

void
Rts2DevClientCamera::readoutEnd ()
{
}

void
Rts2DevClientCamera::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("img_chip"))
    {
      switch (state->
	      value & (CAM_MASK_EXPOSE | CAM_MASK_READING | CAM_MASK_DATA))
	{
	case CAM_EXPOSING:
	  exposureStarted ();
	  break;
	case CAM_DATA:
	  exposureEnd ();
	  break;
	case CAM_NODATA | CAM_NOTREADING | CAM_NOEXPOSURE:
	  readoutEnd ();
	  break;
	}
    }
  Rts2DevClient::stateChanged (state);
}

Rts2DevClientTelescope::Rts2DevClientTelescope (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueDouble ("longtitude"));
  addValue (new Rts2ValueDouble ("latitude"));
  addValue (new Rts2ValueDouble ("altitude"));
  addValue (new Rts2ValueDouble ("ra"));
  addValue (new Rts2ValueDouble ("dec"));
  addValue (new Rts2ValueDouble ("siderealtime"));
  addValue (new Rts2ValueDouble ("localtime"));
  addValue (new Rts2ValueInteger ("flip"));
  addValue (new Rts2ValueDouble ("axis0_counts"));
  addValue (new Rts2ValueDouble ("axis1_counts"));
  addValue (new Rts2ValueInteger ("correction_mark"));
}

void
Rts2DevClientTelescope::getEqu (struct ln_equ_posn *tel)
{
  tel->ra = getValueDouble ("ra");
  tel->dec = getValueDouble ("dec");
}

void
Rts2DevClientTelescope::getObs (struct ln_lnlat_posn *obs)
{
  obs->lng = getValueDouble ("longtitude");
  obs->lat = getValueDouble ("latitude");
}

double
Rts2DevClientTelescope::getLocalSiderealDeg ()
{
  return getValueDouble ("siderealtime") * 15.0;
}

void
Rts2DevClientTelescope::stateChanged (Rts2ServerState * state)
{
  if (state->isName ("telescope"))
    {
      switch (state->value & TEL_MASK_MOVING)
	{
	case TEL_MOVING:
	case TEL_PARKING:
	  moveStart ();
	  break;
	case TEL_OBSERVING:
	case TEL_PARKED:
	  moveEnd ();
	  break;
	}
    }
  Rts2DevClient::stateChanged (state);
}

void
Rts2DevClientTelescope::moveStart ()
{
}

void
Rts2DevClientTelescope::moveEnd ()
{
}

Rts2DevClientDome::Rts2DevClientDome (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("dome"));
  addValue (new Rts2ValueDouble ("temperature"));
  addValue (new Rts2ValueDouble ("humidity"));
  addValue (new Rts2ValueInteger ("power_telescope"));
  addValue (new Rts2ValueInteger ("power_cameras"));
  addValue (new Rts2ValueTime ("next_open"));
  addValue (new Rts2ValueTime ("last_status"));
  addValue (new Rts2ValueInteger ("rain"));
  addValue (new Rts2ValueDouble ("windspeed"));
  addValue (new Rts2ValueDouble ("observingPossible"));
}

Rts2DevClientPhot::Rts2DevClientPhot (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueDouble ("filter"));
  addValue (new Rts2ValueDouble ("filter_c"));
}

Rts2DevClientFocus::Rts2DevClientFocus (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueDouble ("temp"));
  addValue (new Rts2ValueInteger ("pos"));
}

Rts2DevClientExecutor::Rts2DevClientExecutor (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("current"));
  addValue (new Rts2ValueInteger ("next"));
  addValue (new Rts2ValueInteger ("obsid"));
  addValue (new Rts2ValueInteger ("script_count"));
}

Rts2DevClientSelector::Rts2DevClientSelector (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
}

Rts2DevClientImgproc::Rts2DevClientImgproc (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("que_size"));
  addValue (new Rts2ValueInteger ("good_images"));
  addValue (new Rts2ValueInteger ("trash_images"));
  addValue (new Rts2ValueInteger ("morning_images"));
}

int
Rts2DevClientImgproc::command ()
{
  // image ready value
  if (connection->isCommand ("finished_ok"))
    {
      int tar_id;
      int img_id;
      double ra;
      double dec;
      double ra_err;
      double dec_err;
      if (connection->paramNextInteger (&tar_id)
	  || connection->paramNextInteger (&img_id)
	  || connection->paramNextDouble (&ra)
	  || connection->paramNextDouble (&dec)
	  || connection->paramNextDouble (&ra_err)
	  || connection->paramNextDouble (&dec_err)
	  || !connection->paramEnd ())
	return -3;
      connection->getMaster ()->postEvent (new Rts2Event (EVENT_IMAGE_OK));
      return -1;
    }
  return Rts2DevClient::command ();
}

Rts2DevClientGrb::Rts2DevClientGrb (Rts2Conn * in_connection):Rts2DevClient
  (in_connection)
{
  addValue (new Rts2ValueInteger ("last_packet"));
  addValue (new Rts2ValueDouble ("delta"));
  addValue (new Rts2ValueString ("last_target"));
  addValue (new Rts2ValueDouble ("last_target_time"));
}
