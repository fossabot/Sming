/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#include "HttpServer.h"

#include "TcpClient.h"
#include "../Wiring/WString.h"

HttpServer::HttpServer()
{
	settings.keepAliveSeconds = 10;
	configure(settings);
}

HttpServer::HttpServer(HttpServerSettings settings)
{
	configure(settings);
}

void HttpServer::configure(HttpServerSettings settings) {
	this->settings = settings;
	if(settings.minHeapSize != -1 && settings.minHeapSize > -1) {
		minHeapSize = settings.minHeapSize;
	}
	setTimeOut(settings.keepAliveSeconds);
#ifdef ENABLE_SSL
	sslSessionCacheSize = settings.sslSessionCacheSize;
#endif
}

HttpServer::~HttpServer()
{
}

TcpConnection* HttpServer::createClient(tcp_pcb *clientTcp)
{
	HttpServerConnection* con = new HttpServerConnection(clientTcp);
	con->setResourceTree(&resourceTree);
	con->setCompleteDelegate(TcpClientCompleteDelegate(&HttpServer::onConnectionClose, this));

	totalConnections++;
	debugf("Opening connection. Total connections: %d", totalConnections);

	return con;
}

void HttpServer::addPath(String path, const HttpPathDelegate& callback)
{
	if (path.length() > 1 && path.endsWith("/")) {
		path = path.substring(0, path.length() - 1);
	}
	debugf("'%s' registered", path.c_str());

	HttpCompatResource* resource = new HttpCompatResource(callback);
	resourceTree[path] = resource;
}

void HttpServer::setDefaultHandler(const HttpPathDelegate& callback)
{
	addPath("*", callback);
}

void HttpServer::addPath(const String& path, const HttpResourceDelegate& onRequestComplete) {
	HttpResource* resource = new HttpResource;
	resource->onRequestComplete = onRequestComplete;
	resourceTree[path] = resource;
}

void HttpServer::addPath(const String& path, HttpResource* resource) {
	resourceTree[path] = resource;
}

void HttpServer::setDefaultResource(HttpResource* resource) {
	addPath("*", resource);
}

void HttpServer::onConnectionClose(TcpClient& connection, bool success) {
	totalConnections--;
	debugf("Closing connection. Total connections: %d", totalConnections);
}
