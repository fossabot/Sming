/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef SMINGCORE_NETWORK_WEBSOCKETCONNECTION_H_
#define SMINGCORE_NETWORK_WEBSOCKETCONNECTION_H_

#include "../../TcpServer.h"
#include "../HttpServerConnection.h"

#include "../../Services/cWebsocket/websocket.h"

//class CommandExecutor; // TODO: ...

class WebSocketConnection
{
public:
	WebSocketConnection(HttpServerConnection* conn);
	virtual ~WebSocketConnection();

	bool initialize(HttpRequest &request, HttpResponse &response);

	virtual void send(const char* message, int length, wsFrameType type = WS_TEXT_FRAME);
	void sendString(const String& message);
	void sendBinary(const uint8_t* data, int size);
//	void enableCommand(); // TODO: ...
	void close();

	void setUserData(void* userData);
	void* getUserData();

protected:
	bool is(HttpServerConnection* conn) { return connection == conn; };

private:
	void *userData = nullptr;
	HttpServerConnection* connection = nullptr;
//	CommandExecutor* commandExecutor = nullptr; // TODO: ...
};

#endif /* SMINGCORE_NETWORK_WEBSOCKETCONNECTION_H_ */
