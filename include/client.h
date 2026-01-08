/* 
 * Supporting classes for clients programs.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_CLIENT__
#define __RTS2_CLIENT__

#include "block.h"
#include "command.h"
#include "devclient.h"

/**
 * @defgroup RTS2Client
 *
 * This module includes infrastructure needed for client applications.
 */

namespace rts2core
{

/**
 * Represents connection between device and client.
 *
 * This class is used in Rts2Client to represent connections
 * which client establish to other RTS2 components.
 *
 * @ingroup RTS2Client
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnClient:public Connection
{
	private:
		NetworkAddress * address;
		time_t nextTime;  // Next reconnection attempt time
		bool autoReconnect;  // Enable/disable automatic reconnection
		int reconnectTime;  // Reconnection interval in seconds (default: 10)

	protected:
		virtual void connConnected ();
		virtual bool canDelete () { return !autoReconnect; }  // Keep for reconnection if enabled
		virtual void connectionError (int last_data_size);

	public:
		ConnClient (Block *_master, int _centrald_num, char *_name);
		virtual ~ ConnClient (void);

		virtual int init ();
		virtual int idle ();

		virtual void setAddress (NetworkAddress * in_addr);

		/**
		 * Enable/disable automatic reconnection for this device connection.
		 * @param enable True to enable reconnection (disabled by default)
		 */
		void setAutoReconnect (bool enable) { autoReconnect = enable; }

		/**
		 * Check if automatic reconnection is enabled.
		 * @return True if reconnection is enabled
		 */
		bool getAutoReconnect () const { return autoReconnect; }

		/**
		 * Set reconnection time interval.
		 * @param time Reconnection interval in seconds (default: 10)
		 */
		void setReconnectTime (int time) { reconnectTime = time; }

		/**
		 * Get reconnection time interval.
		 * @return Reconnection interval in seconds
		 */
		int getReconnectTime () const { return reconnectTime; }

		/**
		 * Set client key.
		 *
		 * Keys are distributed from Rts2Centrald to client connection.
		 *
		 * @param in_key New client key.
		 */
		virtual void setKey (int in_key);
};

/**
 * Represents connection between centrald and client.
 *
 * This class is used to represent conection between central
 * server and client. It is used to handle key management etc.
 *
 * @ingroup RTS2Client
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ConnCentraldClient:public Connection
{
	public:
		ConnCentraldClient (Block * in_master, const char *in_login, const char *in_name, const char *in_password, const char *in_master_host, const char *in_master_port);
		virtual int init ();
		virtual int idle ();

		virtual int command ();

		/**
		 * Set reconnection time interval.
		 * @param time Reconnection interval in seconds (default: 10)
		 */
		void setReconnectTime (int time) { reconnectTime = time; }

		/**
		 * Get reconnection time interval.
		 * @return Reconnection interval in seconds
		 */
		int getReconnectTime () const { return reconnectTime; }

	protected:
		virtual void setState (rts2_status_t in_value, char * msg);
		virtual bool canDelete () { return false; }  // Keep for reconnection
		virtual void connectionError (int last_data_size);

	private:
		const char *master_host;
		const char *master_port;

		const char *login;
		const char *password;

		time_t nextTime;  // Next reconnection attempt time
		int reconnectTime;  // Reconnection interval in seconds (default: 10)
};

class CommandLogin:public Command
{
	public:
		CommandLogin (Block * master, const char *in_login, const char *name, const char *in_password);
		virtual int commandReturnOK (Connection * conn);

	private:
		enum { LOGIN_SEND, PASSWORD_SEND, INFO_SEND } state;
		const char *login;
		const char *password;
};

/**
 * Common class for client aplication.
 *
 * Connect to centrald, get names of all devices.
 * Works similary to Device.
 *
 * @ingroup RTS2Block RTS2Client
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Client:public Block
{
	public:
		Client (int in_argc, char **in_argv, const char *_name);
		virtual ~ Client (void);

		virtual int run ();

	protected:
		virtual ConnClient * createClientConnection (int _centrald_num, char *_deviceName);
		virtual Connection *createClientConnection (NetworkAddress * in_addr);
		virtual int willConnect (NetworkAddress * in_addr);

		virtual int processOption (int in_opt);
		virtual int init ();

		virtual ConnCentraldClient *createCentralConn ();

		const char *getCentralLogin ()
		{
			return login;
		}
		const char *getName ()
		{
			return name;
		}
		const char *getCentralPassword ()
		{
			return password;
		}
		const char *getCentralHost ()
		{
			return central_host;
		}
		const char *getCentralPort ()
		{
			return central_port;
		}

	private:
		const char *central_host;
		const char *central_port;
		const char *login;
		const char *password;

		const char *name;
};

}
#endif							 /* ! __RTS2_CLIENT__ */
