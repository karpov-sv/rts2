/*
 * Pure TCP connection.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include "connection/tcp.h"

#include <iomanip>
#include <netdb.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/socket.h>

using namespace rts2core;

ConnTCP::ConnTCP (rts2core::Block *_master, const char *_hostname, int _port):ConnNoSend (_master), hostname (_hostname)
{
	port = _port;
	debug = false;
	reconnectTime = 10;

	// Initialize reconnection members
	reconnectAttempt = 0;
	maxReconnectAttempts = -1;      // Unlimited by default (backward compatible)
	lastDisconnectTime = 0;
	autoReconnect = true;           // Enabled by default (backward compatible)
	useExponentialBackoff = false;  // Disabled by default (backward compatible)
	backoffMultiplier = 2.0;
	maxBackoffTime = 3600.0;        // 1 hour max
	minBackoffTime = reconnectTime; // Same as base reconnect time
}

ConnTCP::ConnTCP (rts2core::Block *_master, int _port):ConnNoSend (_master), hostname ("")
{
	port = _port;
	debug = false;
	reconnectTime = 60;

	// Initialize reconnection members
	reconnectAttempt = 0;
	maxReconnectAttempts = -1;      // Unlimited by default (backward compatible)
	lastDisconnectTime = 0;
	autoReconnect = true;           // Enabled by default (backward compatible)
	useExponentialBackoff = false;  // Disabled by default (backward compatible)
	backoffMultiplier = 2.0;
	maxBackoffTime = 3600.0;        // 1 hour max
	minBackoffTime = reconnectTime; // Same as base reconnect time
}

bool ConnTCP::checkBufferForChar (std::istringstream **_is, char end_char)
{
	// look for endchar in received data..
	for (char *p = buf; p < buf_top; p++)
	{
		if (*p == end_char)
		{
			*p = '\0';
			*_is = new std::istringstream (std::string (buf));
			if (p != buf_top)
			{
				memmove (buf, p + 1, buf_top - p - 1);
				buf_top -= p + 1 - buf;
			}
			return true;
		}
	}
	return false;
}

int ConnTCP::init (bool reportConn)
{
	int ret;
	struct sockaddr_in apc_addr;
	struct hostent *hp;

	if (sock != -1)
	{
		logStream (MESSAGE_ERROR) << "TCP connection already connected: " << sock << sendLog;
		return 0;
	}

	sock = socket (AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
		throw ConnCreateError (this, "cannot create socket for TCP/IP connection", errno);

	// empty hostname opens server connection (listening socket)
	if (hostname.length () == 0)
	{
		struct sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_port = htons (port);
		server.sin_addr.s_addr = htonl (INADDR_ANY);
		// bind
		ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
		if (ret)
			throw ConnCreateError (this, "cannot bind on socket", errno);

		// listen
		ret = listen (sock, 1);
		if (ret)
			throw ConnCreateError (this, "cannot listen on socket", errno);

		if (port == 0)
		{
			socklen_t sock_size = sizeof (server);
			ret = getsockname (sock, (struct sockaddr *) &server, &sock_size);
			if (ret)
				throw ConnCreateError (this, "cannot get listen port address", errno);
			port = ntohs (server.sin_port);
		}
	}
	else
	{
        	apc_addr.sin_family = AF_INET;
	        hp = gethostbyname(hostname.c_str ());
		if (hp == NULL)
			throw ConnCreateError (this, (std::string ("unknow hostname ") + hostname).c_str (), errno);

	        bcopy ( hp->h_addr, &(apc_addr.sin_addr.s_addr), hp->h_length);
        	apc_addr.sin_port = htons(port);

		for (int i = 0; i < 3; i++)
		{
        		ret = connect (sock, (struct sockaddr *) &apc_addr, sizeof(apc_addr));
			if (ret == 0)
				break;
		        if (ret == -1 && errno != ENETUNREACH)
			 	throw ConnCreateError (this, "cannot connect socket", errno);
			logStream (MESSAGE_WARNING) << "received network unreachable, waiting 1 second to try again" << sendLog;
			sleep (1);
		}
	}

        ret = fcntl (sock, F_SETFL, O_NONBLOCK);
        if (ret)
		throw ConnCreateError (this, "cannot set socket non-blocking", errno);

	if (hostname.length () == 0)
	{
		setConnState (CONN_CONNECTING);
		logStream (MESSAGE_INFO) << "opened listening port " << port << sendLog;
	}
	else
	{
		setConnState (CONN_CONNECTED);

		if (reconnectAttempt > 0 && reportConn)
		{
			logStream (MESSAGE_INFO)
				<< "Reconnected to " << hostname << ":" << port
				<< " after " << reconnectAttempt << " attempts" << sendLog;
			resetReconnectState ();
		}
		else if (reportConn)
		{
			logStream (MESSAGE_INFO) << "connected to " << hostname << ":" << port << " socket " << sock << sendLog;
		}
	}
        return 0;
}

void ConnTCP::sendData (const void *data, int len, bool binary)
{
	int rest = len;
	if (sock < 0)
		throw ConnError (this, "socket does not exists");

	while (rest > 0)
	{
		int ret;
		ret = send (sock, (char *) data + (len - rest), rest, 0);
		if (ret == -1)
		{
		  	if (errno == EINTR)
				continue;
			if (debug)
			{
				LogStream ls = logStream (MESSAGE_DEBUG);
				ls << "failed to send ";
				if (binary)
					ls.logArrAsHex ((char *) data, len);
				else
				  	ls << data;
				ls << sendLog;
			}
			throw ConnSendError (this, "cannot send data", errno);
		}
		rest -= ret;
	}
	if (debug)
	{
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "send ";
		if (binary)
			ls.logArrAsHex ((char *) data, len);
		else
		  	ls << (char *) data;
		ls << sendLog;
	}
}

void ConnTCP::sendData (const char *data)
{
	sendData ((void *) data, strlen (data), false);
}

void ConnTCP::sendData (std::string data)
{
	sendData ((void *) data.c_str (), data.length (), false);
}

void ConnTCP::receiveData (void *data, size_t len, int wtime, bool binary)
{
	int rest = len;

	fd_set read_set;

	struct timeval read_tout;
	read_tout.tv_sec = wtime;
	read_tout.tv_usec = 0;

	while (rest > 0)
	{
		FD_ZERO (&read_set);

		FD_SET (sock, &read_set);

		int ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
		if (ret < 0)
			throw ConnError (this, "error calling select function", errno);
		else if (ret == 0)
		  	throw ConnTimeoutError (this, "timeout during receiving data");

		// read from descriptor
		ret = recv (sock, (char *)data + (len - rest), rest, 0);
		if (ret == -1)
			throw ConnReceivingError (this, "cannot read from TCP/IP connection", errno);
		rest -= ret;
	}

	if (debug)
	{
		LogStream ls = logStream (MESSAGE_DEBUG);
		ls << "recv ";
		if (binary)
			ls.logArrAsHex ((char *)data, len);
		else
		  	ls << data;
		ls << sendLog;
	}
}

void ConnTCP::receiveTillEnd (char *data, size_t len, int wtime)
{
	int rest = len;

	fd_set read_set;

	struct timeval read_tout;
	read_tout.tv_sec = wtime;
	read_tout.tv_usec = 0;

	while (rest > 0)
	{
		FD_ZERO (&read_set);

		FD_SET (sock, &read_set);

		int ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
		if (ret < 0)
			throw ConnError (this, "error calling select function", errno);
		else if (ret == 0)
		  	throw ConnTimeoutError (this, "timeout during receiving data");

		// read from descriptor
		ret = recv (sock, (char *)data + (len - rest), rest, 0);
		if (ret == 0)
			break;
		if (ret == -1)
			throw ConnReceivingError (this, "cannot read from TCP/IP connection", errno);
		rest -= ret;
	}

	data[len - rest] = '\0';

	if (debug)
	{
		logStream (MESSAGE_DEBUG) << "recv " << data << sendLog;
	}
}

void ConnTCP::receiveData (std::istringstream **_is, int wtime, char end_char)
{
	// check if buffer contains end character..

	fd_set read_set;

	struct timeval read_tout;
	read_tout.tv_sec = wtime;
	read_tout.tv_usec = 0;

	buf_top = buf;

	while (true)
	{
	 	checkBufferSize ();
		FD_ZERO (&read_set);

		FD_SET (sock, &read_set);

		int ret = select (FD_SETSIZE, &read_set, NULL, NULL, &read_tout);
		if (ret < 0)
			throw ConnError (this, "error calling select function", errno);
		else if (ret == 0)
		  	throw ConnTimeoutError (this, "timeout during receiving data");

		// read from descriptor
		ret = recv (sock, buf_top, buf_size - (buf_top - buf), 0);
		if (ret == -1)
			throw ConnReceivingError (this, "cannot read from TCP/IP connection", errno);
		buf_top += ret;
		if (debug)
		{
			*buf_top = '\0';
			logStream (MESSAGE_DEBUG) << "received " << buf << sendLog;
		}

		// look for end character..
		if (checkBufferForChar (_is, end_char))
			return;
	}
}

int ConnTCP::writeRead (const char* wbuf, int wlen, char *rbuf, int rlen, char endChar, int wtime, bool binary)
{
	std::istringstream *is;
	sendData (wbuf, wlen, binary);
	receiveData (&is, wtime, endChar);
	strncpy (rbuf, is->str ().c_str (), rlen);
	rbuf[rlen] = '\0';
	int ret = is->str ().length ();
	delete is;
	return ret;
}

void ConnTCP::postEvent (Event *event)
{
	switch (event->getType ())
	{
		case EVENT_TCP_RECONECT_TIMER:
			if (event->getArg () != this)
				break;

			logStream (MESSAGE_INFO)
				<< "Attempting reconnection to " << hostname << ":" << port
				<< " (attempt " << reconnectAttempt;
			if (maxReconnectAttempts > 0)
				logStream (MESSAGE_INFO) << "/" << maxReconnectAttempts;
			logStream (MESSAGE_INFO) << ")" << sendLog;

			try
			{
				init (false);  // Don't log here, we'll log below

				// Success!
				logStream (MESSAGE_INFO)
					<< "Successfully reconnected to " << hostname << ":" << port
					<< " after " << reconnectAttempt << " attempts and "
					<< (getNow () - lastDisconnectTime) << "s" << sendLog;
				resetReconnectState ();
			}
			catch (ConnError &er)
			{
				logStream (MESSAGE_WARNING)
					<< "Reconnection attempt " << reconnectAttempt
					<< " failed: " << er << sendLog;
				// connectionError() will be called, scheduling next attempt
			}
			break;
	}
	ConnNoSend::postEvent (event);
}

void ConnTCP::connectionError (int last_data_size)
{
	// Record disconnect time on first error
	if (reconnectAttempt == 0 && autoReconnect && reconnectTime > 0)
	{
		lastDisconnectTime = getNow ();
		logStream (MESSAGE_WARNING)
			<< "Connection to " << hostname << ":" << port
			<< " lost, initiating reconnection" << sendLog;
	}

	// Check if should attempt reconnection
	if (shouldAttemptReconnect ())
	{
		double nextInterval = calculateNextReconnectTime ();
		reconnectAttempt++;

		// Log scheduling
		logStream (MESSAGE_INFO)
			<< "Scheduling reconnection attempt " << reconnectAttempt;
		if (maxReconnectAttempts > 0)
			logStream (MESSAGE_INFO) << "/" << maxReconnectAttempts;
		logStream (MESSAGE_INFO)
			<< " in " << nextInterval << "s (elapsed: "
			<< (getNow () - lastDisconnectTime) << "s)" << sendLog;

		getMaster ()->addTimer (nextInterval, new Event (EVENT_TCP_RECONECT_TIMER, this));
	}
	else if (autoReconnect && reconnectAttempt > 0)
	{
		// Max attempts reached
		logStream (MESSAGE_ERROR)
			<< "Max reconnection attempts (" << maxReconnectAttempts
			<< ") reached for " << hostname << ":" << port
			<< " after " << (getNow () - lastDisconnectTime) << "s" << sendLog;
		resetReconnectState ();
	}

	ConnNoSend::connectionError (last_data_size);
}

bool ConnTCP::shouldAttemptReconnect ()
{
	if (!autoReconnect || reconnectTime <= 0)
		return false;

	conn_state_t state = getConnState ();
	if (state != CONN_BROKEN && state != CONN_UNKNOW)
		return false;

	if (maxReconnectAttempts == 0)
		return false;

	if (maxReconnectAttempts > 0 && reconnectAttempt >= maxReconnectAttempts)
		return false;

	if (hostname.length () == 0)  // Server connections don't reconnect
		return false;

	return true;
}

double ConnTCP::calculateNextReconnectTime ()
{
	if (!useExponentialBackoff)
		return reconnectTime;

	double interval = minBackoffTime;
	for (int i = 0; i < reconnectAttempt; i++)
	{
		interval *= backoffMultiplier;
		if (interval >= maxBackoffTime)
			return maxBackoffTime;
	}
	return interval;
}

void ConnTCP::resetReconnectState ()
{
	reconnectAttempt = 0;
	lastDisconnectTime = 0;
}

double ConnTCP::getTimeSinceDisconnect () const
{
	if (lastDisconnectTime == 0)
		return 0;
	return getNow () - lastDisconnectTime;
}

void ConnTCP::setMaxReconnectAttempts (int maxAttempts)
{
	maxReconnectAttempts = maxAttempts;
}

void ConnTCP::setExponentialBackoff (bool enable, double multiplier, double maxBackoff)
{
	useExponentialBackoff = enable;
	backoffMultiplier = multiplier;
	maxBackoffTime = maxBackoff;
	minBackoffTime = reconnectTime;
}

void ConnTCP::resetReconnect ()
{
	resetReconnectState ();
	logStream (MESSAGE_INFO)
		<< "Reconnection state manually reset for " << hostname << ":" << port
		<< sendLog;
}
