#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#include "status.h"
#include "rts2device.h"
#include "rts2command.h"

#define MINDATAPORT		5556
#define MAXDATAPORT		5656

Rts2LogStream & sendLog (Rts2LogStream & _ls)
{
  _ls.sendLog ();
  return _ls;
}

void
Rts2LogStream::sendLog ()
{
  masterDevice->sendMessage (messageType, ls.str ().c_str ());
}

int
Rts2DevConn::commandAuthorized ()
{
  if (isCommand ("ready"))
    {
      return master->ready (this);
    }
  else if (isCommand ("info"))
    {
      return master->info (this);
    }
  else if (isCommand ("base_info"))
    {
      return master->baseInfo (this);
    }
  else if (isCommand ("killall"))
    {
      CHECK_PRIORITY;
      return master->killAll ();
    }
  else if (isCommand ("script_ends"))
    {
      CHECK_PRIORITY;
      return master->scriptEnds ();
    }
  // pseudo-command; will not be answered with ok ect..
  // as it can occur inside command block
  else if (isCommand ("this_device"))
    {
      char *deviceName;
      int deviceType;
      if (paramNextString (&deviceName)
	  || paramNextInteger (&deviceType) || !paramEnd ())
	return -2;
      setName (deviceName);
      setOtherType (deviceType);
      return -1;
    }
  // we need to try that - due to other device commands
  return -5;
}

int
Rts2DevConn::command ()
{
  int ret;
  if ((getCentraldId () != -1 && isConnState (CONN_AUTH_OK)) || getType () == DEVICE_DEVICE)	// authorized and running
    {
      ret = commandAuthorized ();
      if (ret == DEVDEM_E_HW)
	{
	  sendCommandEnd (DEVDEM_E_HW, "device error");
	  return -1;
	}
      if (ret != -5)
	return ret;
    }
  if (isCommand ("auth"))
    {
      int auth_id;
      int auth_key;
      if (paramNextInteger (&auth_id)
	  || paramNextInteger (&auth_key) || !paramEnd ())
	return -2;
      setCentraldId (auth_id);
      setKey (auth_key);
      ret = master->authorize (this);
      if (ret)
	{
	  sendCommandEnd (DEVDEM_E_SYSTEM,
			  "cannot authorize; try again later");
	  setConnState (CONN_AUTH_FAILED);
	  return -1;
	}
      setConnState (CONN_AUTH_PENDING);
      return -1;
    }
  return Rts2Conn::command ();
}

Rts2DevConn::Rts2DevConn (int in_sock, Rts2Device * in_master):Rts2Conn (in_sock,
	  in_master)
{
  address = NULL;
  master = in_master;
  if (in_sock > 0)
    setType (DEVICE_SERVER);
  else
    setType (DEVICE_DEVICE);
}

int
Rts2DevConn::init ()
{
  if (getType () != DEVICE_DEVICE)
    return Rts2Conn::init ();
  int ret;
  struct addrinfo *device_addr;
  if (!address)
    return -1;

  ret = address->getSockaddr (&device_addr);

  if (ret)
    return ret;
  sock =
    socket (device_addr->ai_family, device_addr->ai_socktype,
	    device_addr->ai_protocol);
  if (sock == -1)
    {
      return -1;
    }
  ret = fcntl (sock, F_SETFL, O_NONBLOCK);
  if (ret == -1)
    {
      return -1;
    }
  ret = connect (sock, device_addr->ai_addr, device_addr->ai_addrlen);
  freeaddrinfo (device_addr);
  if (ret == -1)
    {
      if (errno == EINPROGRESS)
	{
	  setConnState (CONN_CONNECTING);
	  return 0;
	}
      return -1;
    }
  connAuth ();
  return 0;
}

int
Rts2DevConn::idle ()
{
  if (getType () != DEVICE_DEVICE)
    return Rts2Conn::idle ();

  if (isConnState (CONN_CONNECTING))
    {
      int err = 0;
      int ret;
      socklen_t len = sizeof (err);

      ret = getsockopt (sock, SOL_SOCKET, SO_ERROR, &err, &len);
      if (ret)
	{
#ifdef DEBUG_EXTRA
	  syslog (LOG_ERR, "Rts2Dev2DevConn::idle getsockopt %m");
#endif
	  connectionError (-1);
	}
      else if (err)
	{
#ifdef DEBUG_EXTRA
	  syslog (LOG_ERR, "Rts2Dev2DevConn::idle getsockopt %s",
		  strerror (err));
#endif
	  connectionError (-1);
	}
      else
	{
	  connAuth ();
	}
    }
  return Rts2Conn::idle ();
}

int
Rts2DevConn::authorizationOK ()
{
  setConnState (CONN_AUTH_OK);
  master->sendStatusInfo (this);
  master->baseInfo ();
  master->sendBaseInfo (this);
  master->info ();
  master->sendInfo (this);
  sendCommandEnd (0, "OK authorized");
  return 0;
}

int
Rts2DevConn::authorizationFailed ()
{
  setCentraldId (-1);
  setConnState (CONN_DELETE);
  sendCommandEnd (DEVDEM_E_SYSTEM, "authorization failed");
  syslog (LOG_DEBUG, "authorizationFailed: %s", getName ());
  return 0;
}

void
Rts2DevConn::setHavePriority (int in_have_priority)
{
  if (havePriority () != in_have_priority)
    {
      Rts2Conn::setHavePriority (in_have_priority);
      if (havePriority ())
	{
	  send ("S priority 1 priority received");
	}
      else
	{
	  send ("S priority 0 priority lost");
	}
    }
}

void
Rts2DevConn::setDeviceAddress (Rts2Address * in_addr)
{
  address = in_addr;
  setConnState (CONN_CONNECTING);
  setName (in_addr->getName ());
  init ();
  setOtherType (address->getType ());
}

void
Rts2DevConn::setDeviceName (char *in_name)
{
  setName (in_name);
  setConnState (CONN_RESOLVING_DEVICE);
}

void
Rts2DevConn::connAuth ()
{
#ifdef DEBUG_EXTRA
  syslog (LOG_DEBUG, "auth: %s state: %i", getName (), getConnState ());
#endif
  master->getCentraldConn ()->
    queCommand (new Rts2CommandAuthorize (master, getName ()));
  setConnState (CONN_AUTH_PENDING);
}

void
Rts2DevConn::setKey (int in_key)
{
  Rts2Conn::setKey (in_key);
  if (getType () == DEVICE_DEVICE)
    {
      if (isConnState (CONN_AUTH_PENDING))
	{
	  // que to begining, send command
	  // kill all runinng commands
	  queSend (new Rts2CommandSendKey (master, in_key));
	}
      else
	{
#ifdef DEBUG_EXTRA
	  syslog (LOG_DEBUG,
		  "Rts2DevConn::setKey invalid connection state: %i",
		  getConnState ());
#endif
	}
    }
}

void
Rts2DevConn::setConnState (conn_state_t new_conn_state)
{
  if (getType () != DEVICE_DEVICE)
    return Rts2Conn::setConnState (new_conn_state);
  if (new_conn_state == CONN_AUTH_OK)
    {
      char *msg;
      asprintf (&msg, "this_device %s %i", master->getDeviceName (),
		master->getDeviceType ());
      send (msg);
      free (msg);
      master->sendStatusInfo (this);
      master->baseInfo (this);
      master->info (this);
    }
  Rts2Conn::setConnState (new_conn_state);
}

Rts2LogStream Rts2DevConn::logStream (messageType_t in_messageType)
{
  Rts2LogStream
  ls (master, in_messageType);
  return ls;
}


Rts2DevConnMaster::Rts2DevConnMaster (Rts2Block * in_master, char *in_device_host, int in_device_port, char *in_device_name, int in_device_type, char *in_master_host, int in_master_port):
Rts2Conn (-1, in_master)
{
  device_host = in_device_host;
  device_port = in_device_port;
  strncpy (device_name, in_device_name, DEVICE_NAME_SIZE);
  device_type = in_device_type;
  strncpy (master_host, in_master_host, HOST_NAME_MAX);
  master_port = in_master_port;
}

Rts2DevConnMaster::~Rts2DevConnMaster (void)
{
}

int
Rts2DevConnMaster::registerDevice ()
{
  char *msg;
  int ret;
  if (!device_host)
    {
      device_host = new char[HOST_NAME_MAX];
      ret = gethostname (device_host, HOST_NAME_MAX);
      if (ret < 0)
	return -1;
    }
  asprintf (&msg, "register %s %i %s %i", device_name, device_type,
	    device_host, device_port);
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2DevConnMaster::connectionError (int last_data_size)
{
  if (sock > 0)
    {
      close (sock);
      sock = -1;
    }
  if (!isConnState (CONN_BROKEN))
    {
      setConnState (CONN_BROKEN);
      time (&nextTime);
      nextTime += 60;
    }
  return -1;
}

int
Rts2DevConnMaster::init ()
{
  struct hostent *master_info;
  struct sockaddr_in address;
  int ret;
  // init sock address
  address.sin_family = AF_INET;
  address.sin_port = htons (master_port);
  // get remote host
  if ((master_info = gethostbyname (master_host)))
    {
      address.sin_addr = *(struct in_addr *) master_info->h_addr;
    }
  else
    {
      syslog (LOG_ERR, "Rts2DevConnMaster::init cannot get host name %s : %m",
	      master_host);
      return -1;
    }
  // get hostname
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      syslog (LOG_ERR, "Rts2DevConnMaster::init cannot create socket: %m");
      return -1;
    }
  ret = connect (sock, (struct sockaddr *) &address, sizeof (address));
  if (ret < 0)
    {
      syslog (LOG_ERR, "Rts2DevConnMaster::init cannot connect: %m");
      close (sock);
      sock = -1;
      return -1;
    }
  // have to wait for reply
  return registerDevice ();
}

int
Rts2DevConnMaster::idle ()
{
  time_t now;
  time (&now);
  switch (getConnState ())
    {
    case CONN_BROKEN:
      if (now > nextTime)
	{
	  nextTime = now + 60;
	  int ret;
	  ret = init ();
	  if (!ret)
	    setConnState (CONN_AUTH_OK);
	}
      break;
    default:
      break;
    }
  return Rts2Conn::idle ();
}

int
Rts2DevConnMaster::command ()
{
  Rts2Conn *auth_conn;
  if (isCommand ("A"))
    {
      char *a_type;
      int auth_id;
      if (paramNextString (&a_type))
	return -1;
      if (!strcmp ("authorization_ok", a_type))
	{
	  if (paramNextInteger (&auth_id) || !paramEnd ())
	    return -1;
	  // find connection with given ID
	  auth_conn = master->findCentralId (auth_id);
	  if (auth_conn)
	    auth_conn->authorizationOK ();
	  return -1;
	}
      else if (!strcmp ("authorization_failed", a_type))
	{
	  if (paramNextInteger (&auth_id) || !paramEnd ())
	    return -1;
	  // find connection with given ID
	  auth_conn = master->findCentralId (auth_id);
	  if (auth_conn)
	    auth_conn->authorizationFailed ();
	  return -1;
	}
      if (!strcmp ("registered_as", a_type))
	{
	  int clientId;
	  if (paramNextInteger (&clientId) || !paramEnd ())
	    return -2;
	  setCentraldId (clientId);
	  return -1;
	}
    }
  if (isCommand ("authorization_key"))
    {
      char *p_device_name;
      int p_key;
      Rts2Conn *conn;
      if (paramNextString (&p_device_name)
	  || paramNextInteger (&p_key) || !paramEnd ())
	return -2;
      conn = master->getOpenConnection (p_device_name);
      if (conn)
	conn->setKey (p_key);
      return -1;
    }

  return Rts2Conn::command ();
}

int
Rts2DevConnMaster::priorityChange ()
{
  // change priority
  int priority_client;
  int timeout;
  if (paramNextInteger (&priority_client) || paramNextInteger (&timeout))
    return -2;
  master->setPriorityClient (priority_client, timeout);
  return -1;
}

int
Rts2DevConnMaster::informations ()
{
  char *i_name;
  int status_num;
  char *state_name;
  int state_value;
  if (paramNextString (&i_name) || paramNextInteger (&status_num)
      || paramNextString (&state_name) || paramNextInteger (&state_value)
      || !paramEnd ())
    return -1;
  return master->setMasterState (state_value);
}

int
Rts2DevConnMaster::status ()
{
  char *msg;
  int new_state;
  if (paramNextString (&msg) || paramNextInteger (&new_state) || !paramEnd ())
    return -1;
  return master->setMasterState (new_state);
}

int
Rts2DevConnMaster::authorize (Rts2DevConn * conn)
{
  char *msg;
  int ret;
  asprintf (&msg, "authorize %i %i", conn->getCentraldId (), conn->getKey ());
  ret = send (msg);
  free (msg);
  return ret;
}

int
Rts2DevConnData::init ()
{
  // find empty port
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return -1;

  struct sockaddr_in server;
  int test_port;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);

  // find empty port
  for (test_port = MINDATAPORT; test_port < MAXDATAPORT; test_port++)
    {
      server.sin_port = htons (test_port);
      if (bind (sock, (struct sockaddr *) &server, sizeof (server)) == 0)
	break;
    }
  if (test_port == MAXDATAPORT)
    {
      close (sock);
      sock = -1;
      return -1;
    }

  if (listen (sock, 1))
    {
      close (sock);
      sock = -1;
      return -1;
    }

  setPort (test_port);

  setConnState (CONN_CONNECTING);
  return 0;
}

int
Rts2DevConnData::send (const char *msg)
{
  return 0;
}

int
Rts2DevConnData::send (char *data, size_t data_size)
{
  if (!isConnState (CONN_CONNECTED))
    return -2;
  return write (sock, data, data_size);
}

int
Rts2DevConnData::sendHeader ()
{
  return 0;
}

void
Rts2State::setState (int new_state, char *description)
{
  // state was set..do not set it again
  if (state == new_state)
    {
      return;
    }
  state = new_state;
  master->sendStatusMessage (state_name, state);
};

void
Rts2State::maskState (int state_mask, int new_state, char *description)
{
  int masked_state = state;
  // null from state all errors..
  masked_state &= ~(DEVICE_ERROR_MASK | state_mask);
  masked_state |= new_state;
  setState (masked_state, description);
}

int
Rts2State::sendInfo (Rts2Conn * conn, int state_num)
{
  int ret;
  char *msg;
  asprintf (&msg, "I status %i %s %i", state_num, state_name, state);
  ret = conn->send (msg);
  free (msg);
  return ret;
}

Rts2Device::Rts2Device (int in_argc, char **in_argv, int in_device_type, char *default_name):
Rts2Daemon (in_argc, in_argv)
{
  /* put defaults to variables.. */
  device_name = default_name;
  centrald_host = "localhost";
  centrald_port = 617;
  log_option = 0;

  statesSize = 0;
  states = NULL;

  device_type = in_device_type;

  device_host = NULL;

  timerclear (&info_time);

  lockf = 0;

  idleInfoInterval = -1;
  nextIdleInfo = 0;

  // now add options..
  addOption ('L', "hostname", 1,
	     "hostname, if it different from return of gethostname()");
  addOption ('S', "centrald_host", 1,
	     "name of computer, on which central server runs");
  addOption ('P', "centrald_port", 1, "port number of central host");
  addOption ('M', "mail-to", 1, "send report mails to this adresses");
  addOption ('d', "device_name", 1, "name of device");
  addOption ('e', "log_stderr", 0,
	     "logs also to stderr (not only to syslogd)");
}

Rts2Device::~Rts2Device (void)
{
  int i;

  if (lockf)
    close (lockf);

  for (i = 0; i < statesSize; i++)
    delete states[i];
  free (states);
  states = NULL;
}

Rts2DevConn *
Rts2Device::createConnection (int in_sock, int conn_num)
{
  return new Rts2DevConn (in_sock, this);
}

void
Rts2Device::setStateNames (int in_states_size, char **states_names)
{
  if (in_states_size == 0)
    return;

  int i;
  char *state_name = *states_names;
#ifdef DEBUG_ALL
  syslog (LOG_DEBUG, "Rts2Device::setStateNames states: %i\n",
	  in_states_size);
#endif
  states = (Rts2State **) malloc (sizeof (Rts2State *) * in_states_size);
  for (i = 0; i < in_states_size; i++)
    {
      states[i] = new Rts2State (this, state_name);
      state_name++;
    }
  statesSize = in_states_size;
}

int
Rts2Device::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'L':
      device_host = optarg;
      break;
    case 'S':
      centrald_host = optarg;
      break;
    case 'P':
      centrald_port = atoi (optarg);
      break;
    case 'M':
      mailAddress = optarg;
      break;
    case 'd':
      device_name = optarg;
      break;
    case 'e':
      log_option |= LOG_PERROR;
      break;
    default:
      return Rts2Daemon::processOption (in_opt);
    }
  return 0;
}

void
Rts2Device::clearStatesPriority ()
{
  int i;
  for (i = 0; i < statesSize; i++)
    {
      maskState (i, 0xffffff | DEVICE_ERROR_MASK, DEVICE_ERROR_KILL,
		 "all operations canceled by priority");
    }
}

Rts2Conn *
Rts2Device::createClientConnection (char *in_device_name)
{
  Rts2DevConn *conn;
  conn = createConnection (-1, -1);
  conn->setDeviceName (in_device_name);
  return conn;
}

Rts2Conn *
Rts2Device::createClientConnection (Rts2Address * in_addres)
{
  Rts2DevConn *conn;
  if (in_addres->isAddress (device_name))
    return NULL;
  conn = createConnection (-1, -1);
  conn->setDeviceAddress (in_addres);
  return conn;
}

int
Rts2Device::changeState (int state_num, int new_state, char *description)
{
  states[state_num]->setState (new_state, description);
  return 0;
}

int
Rts2Device::maskState (int state_num, int state_mask, int new_state,
		       char *description)
{
#ifdef DEBUG_EXTRA
  syslog (LOG_DEBUG,
	  "Rts2Device::maskState state: %i state_mask: %i new_state: %i desc: %s",
	  state_num, state_mask, new_state, description);
#endif
  states[state_num]->maskState (state_mask, new_state, description);
  return 0;
}

int
Rts2Device::init ()
{
  int ret;
  char *lock_fname;
  FILE *lock_file;

  // try to open log file..

  ret = Rts2Daemon::init ();
  if (ret)
    return ret;

  openlog (NULL, log_option, LOG_LOCAL0);

  asprintf (&lock_fname, "/var/run/rts2_%s", device_name);

  lockf = open (lock_fname, O_RDWR | O_CREAT);

  if (lockf == -1)
    {
      syslog (LOG_ERR, "cannot open lock file %s", lock_fname);
      return -1;
    }

  ret = flock (lockf, LOCK_EX | LOCK_NB);
  if (ret)
    {
      if (errno == EWOULDBLOCK)
	{
	  syslog (LOG_ERR, "lock file %s owned by another process",
		  lock_fname);
	  return -1;
	}
      syslog (LOG_ERR, "cannot flock %s: %m, lock_fname", lock_fname);
      return -1;
    }

  lock_file = fdopen (lockf, "w+");

  free (lock_fname);

  fprintf (lock_file, "%i\n", getpid ());

  fflush (lock_file);

  conn_master =
    new Rts2DevConnMaster (this, device_host, getPort (), device_name,
			   device_type, centrald_host, centrald_port);
  connections[0] = conn_master;

  while (connections[0]->init () < 0)
    {
      syslog (LOG_DEBUG, "Rts2Device::init waiting for master");
      sleep (60);
    }
  return ret;
}

int
Rts2Device::idle ()
{
  time_t now = time (NULL);
  if (idleInfoInterval >= 0 && now > nextIdleInfo)
    {
      infoAll ();
    }
  return Rts2Daemon::idle ();
}

int
Rts2Device::authorize (Rts2DevConn * conn)
{
  return conn_master->authorize (conn);
}

int
Rts2Device::sendStatusInfo (Rts2DevConn * conn)
{
  int i;
  int ret;

  for (i = 0; i < statesSize; i++)
    {
      ret = states[i]->sendInfo (conn, i);
      if (ret)
	return ret;
    }
  return conn->sendPriorityInfo (i);
}

int
Rts2Device::ready ()
{
  return -1;
}

int
Rts2Device::sendInfo (Rts2Conn * conn)
{
  conn->sendValue ("info_time",
		   (double) (info_time.tv_sec +
			     info_time.tv_usec / USEC_SEC));
  return 0;
}

int
Rts2Device::info ()
{
  gettimeofday (&info_time, NULL);
  return 0;
}

int
Rts2Device::baseInfo ()
{
  return -1;
}

void
Rts2Device::sendMessage (messageType_t in_messageType,
			 const char *in_messageString)
{
  Rts2Conn *centraldConn = getCentraldConn ();
  if (!centraldConn)
    return;
  Rts2Message msg =
    Rts2Message (getDeviceName (), in_messageType, in_messageString);
  centraldConn->sendMessage (msg);
}

void
Rts2Device::sendMessage (messageType_t in_messageType,
			 std::ostringstream & _os)
{
  sendMessage (in_messageType, _os.str ().c_str ());
}

Rts2LogStream
Rts2Device::logStream (messageType_t in_messageType)
{
  Rts2LogStream ls (this, in_messageType);
  return ls;
}

int
Rts2Device::killAll ()
{
  cancelPriorityOperations ();
  return 0;
}

int
Rts2Device::scriptEnds ()
{
  return 0;
}

int
Rts2Device::ready (Rts2Conn * conn)
{
  int ret;
  ret = ready ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
      return -1;
    }
  return 0;
}

int
Rts2Device::info (Rts2Conn * conn)
{
  int ret;
  ret = info ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
      return -1;
    }
  return sendInfo (conn);
}

int
Rts2Device::infoAll ()
{
  Rts2Conn *conn;
  int ret;
  nextIdleInfo = time (NULL) + idleInfoInterval;
  ret = info ();
  if (ret)
    return -1;
  for (int i = 0; i < MAX_CONN; i++)
    {
      conn = connections[i];
      if (conn)
	{
	  sendInfo (conn);
	}
    }
  return 0;
}

int
Rts2Device::baseInfo (Rts2Conn * conn)
{
  int ret;
  ret = baseInfo ();
  if (ret)
    {
      conn->sendCommandEnd (DEVDEM_E_HW, "device not ready");
      return -1;
    }
  return sendBaseInfo (conn);
}
