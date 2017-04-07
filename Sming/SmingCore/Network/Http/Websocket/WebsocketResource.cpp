/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 *
 * @author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#include "WebsocketResource.h"

WebsocketResource::WebsocketResource() {
	onHeadersComplete = HttpResourceDelegate(&WebsocketResource::checkHeaders, this);
	onBody = HttpServerConnectionBodyDelegate(&WebsocketResource::processWebSocketFrame, this);
}

WebsocketResource::~WebsocketResource() {
	if (sock != NULL) {
		delete sock;
	}
}

int WebsocketResource::checkHeaders(HttpServerConnection& connection, HttpRequest& request, HttpResponse& response) {
	if (sock != NULL) {
		delete sock;
	}

	sock = new WebSocketConnection(&connection);
	if (!sock->initialize(request, response)) {
		debugf("Not a valid WebsocketRequest?");
		return -1;
	}

	connection.setTimeOut(USHRT_MAX); //Disable disconnection on connection idle (no rx/tx)

// TODO: Re-Enable Command Executor...

	memset(&parserSettings, 0, sizeof(parserSettings));
	parserSettings.on_data_begin = staticOnDataBegin;
	parserSettings.on_data_payload = staticOnDataPayload;
	parserSettings.on_data_end = staticOnDataEnd;
	parserSettings.on_control_begin = staticOnControlBegin;
	parserSettings.on_control_payload = staticOnControlPayload;
	parserSettings.on_control_end = staticOnControlEnd;

	ws_parser_init(&parser, &parserSettings);
	parser.user_data = (void*)this;

	return 0;
}

int WebsocketResource::processWebSocketFrame(HttpServerConnection& connection, HttpRequest& request, const char *at, int size) {
	int rc = ws_parser_execute(&parser, (char *)at, size);
	if (rc != WS_OK) {
		debugf("WebSocketResource error: %d %s\n", rc, ws_parser_error(rc));
		return -1;
	}

	return 0;
}

void WebsocketResource::setConnectionHandler(WebSocketDelegate handler) {
	wsConnect = handler;
}

void WebsocketResource::setMessageHandler(WebSocketMessageDelegate handler) {
	wsMessage = handler;
}

void WebsocketResource::setBinaryHandler(WebSocketBinaryDelegate handler) {
	wsBinary = handler;
}

void WebsocketResource::setDisconnectionHandler(WebSocketDelegate handler) {
	wsDisconnect = handler;
}

int WebsocketResource::staticOnDataBegin(void* userData, ws_frame_type_t type) {
	if (userData == NULL) {
		return -1;
	}

	WebsocketResource *resource = (WebsocketResource *)userData;

	resource->frameType = type;

	debugf("data_begin: %s\n",
			type == WS_FRAME_TEXT ? "text" :
			type == WS_FRAME_BINARY ? "binary" :
			"?");

	return WS_OK;
}

int WebsocketResource::staticOnDataPayload(void* userData, const char *at, size_t length) {
	if (userData == NULL) {
		return -1;
	}

	WebsocketResource *resource = (WebsocketResource *)userData;

	if (resource->frameType == WS_FRAME_TEXT && resource->wsMessage) {
		resource->wsMessage(*resource->sock, String(at, length));
	} else if (resource->frameType == WS_FRAME_BINARY && resource->wsBinary) {
		resource->wsBinary(*resource->sock, (uint8_t *) at, length);
	}

	return WS_OK;
}

int WebsocketResource::staticOnDataEnd(void* userData) {
	return WS_OK;
}

int WebsocketResource::staticOnControlBegin(void* userData, ws_frame_type_t type) {
	// TODO: ping / pong

	return WS_OK;
}

int WebsocketResource::staticOnControlPayload(void* userData, const char *data, size_t length) {
	return WS_OK;
}

int WebsocketResource::staticOnControlEnd(void* userData) {
	return WS_OK;
}

