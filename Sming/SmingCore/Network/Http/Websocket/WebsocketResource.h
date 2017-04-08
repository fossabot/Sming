/*
 * WebResource.h
 *
 *  Created on: Apr 3, 2017
 *      Author: slavey
 */

#ifndef _SMING_SMINGCORE_NETWORK_WEBSOCKET_RESOURCE_H_
#define _SMING_SMINGCORE_NETWORK_WEBSOCKET_RESOURCE_H_

#include "../HttpResource.h"
#include "WebSocketConnection.h"

extern "C" {
	#include "../ws_parser/ws_parser.h"
}

//#include "../../Services/CommandProcessing/CommandProcessingIncludes.h" // TODO: ....

typedef Delegate<void(WebSocketConnection&)> WebSocketDelegate;
typedef Delegate<void(WebSocketConnection&, const String&)> WebSocketMessageDelegate;
typedef Delegate<void(WebSocketConnection&, uint8_t* data, size_t size)> WebSocketBinaryDelegate;

//typedef Vector<WebSocketConnection> WebSocketsList;

class WebsocketResource: public HttpResource {

public:
	WebsocketResource();
	~WebsocketResource();
	int checkHeaders(HttpServerConnection& connection, HttpRequest& request, HttpResponse& response);
	int processWebSocketFrame(HttpServerConnection& connection, HttpRequest& request, const char *at, int size);

	void setConnectionHandler(WebSocketDelegate handler);

	void setMessageHandler(WebSocketMessageDelegate handler);

	void setBinaryHandler(WebSocketBinaryDelegate handler);
	void setDisconnectionHandler(WebSocketDelegate handler);

protected:
	static int staticOnDataBegin(void* userData, ws_frame_type_t type);
	static int staticOnDataPayload(void* userData, const char *at, size_t length);
	static int staticOnDataEnd(void* userData);
	static int staticOnControlBegin(void* userData, ws_frame_type_t type);
	static int staticOnControlPayload(void* userData, const char*, size_t length);
	static int staticOnControlEnd(void* userData);

protected:
	WebSocketConnection *sock = NULL;

	ws_frame_type_t frameType = WS_FRAME_TEXT;

	ws_parser_t parser;
	ws_parser_callbacks_t parserSettings;

	WebSocketDelegate wsConnect;
	WebSocketMessageDelegate wsMessage;
	WebSocketBinaryDelegate wsBinary;
	WebSocketDelegate wsDisconnect;
};

#endif /* _SMING_SMINGCORE_NETWORK_WEBSOCKET_RESOURCE_H_ */
