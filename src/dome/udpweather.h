/**********************************************************************************************
 *
 * Class for weather info connection.
 *
 * Bind to specified port, send back responds packet, changed state
 * acordingly to weather service, close/open dome as well (using
 * master pointer)
 *
 * To be used in Pierre-Auger site in Argentina.
 * 
 *********************************************************************************************/

#include <sys/types.h>
#include <time.h>

#include "dome.h"

// how long after weather was bad can weather be good again; in
// seconds
#define FRAM_BAD_WEATHER_TIMEOUT   7200
#define FRAM_BAD_WINDSPEED_TIMEOUT 600
#define FRAM_CONN_TIMEOUT	   600

// how long we will keep lastWeatherStatus as actual (in second)
#define FRAM_WEATHER_TIMEOUT	40
// should be in 0-99 range, as 99 is maximum value which station can measure
#define FRAM_MAX_WINDSPEED      50
#define FRAM_MAX_PEAK_WINDSPEED 50

class Rts2ConnFramWeather:public Rts2Conn
{
private:
  Rts2DevDome * master;
  int weather_port;

  int rain;
  float windspeed;
  time_t lastWeatherStatus;
  time_t lastBadWeather;
  time_t nextGoodWeather;

  void setWeatherTimeout (time_t wait_time);

protected:

public:
    Rts2ConnFramWeather (int in_weather_port, Rts2DevDome * in_master);
  virtual int init ();
  virtual int receive (fd_set * set);
  // return 1 if weather is favourable to open dome..
  virtual int isGoodWeather ();
};
