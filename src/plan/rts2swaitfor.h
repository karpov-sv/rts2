#ifndef __RTS2_SWAITFOR__
#define __RTS2_WAITFOR__

#include "rts2script.h"

class Rts2SWaitFor:public Rts2ScriptElement
{
private:
  std::string deviceName;
  std::string valueName;
  double tarval;
  double range;
protected:
    virtual void getDevice (char new_device[DEVICE_NAME_SIZE]);
public:
    Rts2SWaitFor (Rts2Script * in_script, const char *new_device,
		  char *valueName, double value, double range);
  virtual int defnextCommand (Rts2DevClient * client,
			      Rts2Command ** new_command,
			      char new_device[DEVICE_NAME_SIZE]);
  virtual int idle ();
};

#endif /* !__RTS2_WAITFOR__ */
