#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "rts2connnosend.h"

Rts2ConnNoSend::Rts2ConnNoSend (Rts2Block * in_master):Rts2Conn (in_master)
{
  setConnTimeout (-1);
}

Rts2ConnNoSend::~Rts2ConnNoSend (void)
{
}

int
Rts2ConnNoSend::send (char *messageg)
{
  return 1;
}
