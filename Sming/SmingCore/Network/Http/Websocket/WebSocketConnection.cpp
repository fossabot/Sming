/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#include "WebSocketConnection.h"
#include "../../Services/WebHelpers/aw-sha1.h"
#include "../../Services/WebHelpers/base64.h"
//#include "../../Services/CommandProcessing/CommandExecutor.h"

WebSocketConnection::WebSocketConnection(HttpServerConnection* conn)
{
	connection = conn;
}

WebSocketConnection::~WebSocketConnection()
{
//#if ENABLE_CMD_EXECUTOR
//	if (commandExecutor)
//	{
//		delete commandExecutor;
//	}
//#endif
}

bool WebSocketConnection::initialize(HttpRequest& request, HttpResponse& response)
{
	String version = request.getHeader("Sec-WebSocket-Version");
	version.trim();
	if (version.toInt() != 13) // 1.3
		return false;

	String hash = request.getHeader("Sec-WebSocket-Key");
	hash.trim();
	hash = hash + secret;
	unsigned char data[SHA1_SIZE];
	char secure[SHA1_SIZE * 4];
	sha1(data, hash.c_str(), hash.length());
	base64_encode(SHA1_SIZE, data, SHA1_SIZE * 4, secure);
	response.code = HTTP_STATUS_SWITCHING_PROTOCOLS;
	response.setHeader("Connection", "Upgrade");
	response.setHeader("Upgrade", "websocket");
	response.setHeader("Sec-WebSocket-Accept", secure);
	return true;
}

void WebSocketConnection::send(const char* message, int length, wsFrameType type)
{
	uint8_t frameHeader[16] = {0};
	size_t headSize = sizeof(frameHeader);
	wsMakeFrame(nullptr, length, frameHeader, &headSize, type);
	connection->send((const char* )frameHeader, (uint16_t )headSize);
	connection->send((const char* )message, (uint16_t )length);
}

void WebSocketConnection::sendString(const String& message)
{
	send(message.c_str(), message.length());
}

void WebSocketConnection::sendBinary(const uint8_t* data, int size)
{
	send((char*)data, size, WS_BINARY_FRAME);
}

//void WebSocketConnection::enableCommand()
//{
//#if ENABLE_CMD_EXECUTOR
//	if (!commandExecutor)
//	{
//		commandExecutor = new CommandExecutor(this);
//	}
//#endif
//}

void WebSocketConnection::close()
{
	connection->close();
}

void WebSocketConnection::setUserData(void* userData)
{
	this->userData = userData;
}

void* WebSocketConnection::getUserData()
{
	return userData;
}
