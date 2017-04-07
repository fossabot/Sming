/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef _SMING_CORE_HTTPSERVER_H_
#define _SMING_CORE_HTTPSERVER_H_

#include "TcpServer.h"
#include "../Wiring/WString.h"
#include "../Wiring/WHashMap.h"
#include "../Delegate.h"
#include "Http/HttpResponse.h"
#include "Http/HttpRequest.h"
#include "Http/HttpResource.h"
#include "Http/HttpServerConnection.h"

typedef Delegate<void(HttpRequest&, HttpResponse&)> HttpPathDelegate;
typedef Delegate<int(HttpServerConnection&, HttpRequest&, HttpResponse&)> HttpResourceDelegate;

typedef struct {
	int maxActiveConnections = 10; // << the maximum number of concurrent requests..
	int keepAliveSeconds = 5; // << the default seconds to keep the connection alive before closing it
} HttpServerSettings;


class HttpServer: public TcpServer
{
	friend class HttpServerConnection;

public:
	HttpServer();
	HttpServer(HttpServerSettings settings);
	virtual ~HttpServer();

	/*
	 * @brief Allows changing the server configuration
	 */
	void configure(HttpServerSettings settings);

	void addPath(String path, HttpPathDelegate callback);
	void setDefaultHandler(HttpPathDelegate callback);

	void addPath(const String& path, HttpResourceDelegate onRequestComplete);

	void addPath(const String& path, const HttpResource& resource);
	void setDefaultResource(const HttpResource& resource);

protected:
	virtual TcpConnection* createClient(tcp_pcb *clientTcp);
	virtual void onConnectionClose(TcpClient& connection, bool success);

private:
	HttpServerSettings settings;
	ResourceTree resourceTree;
};

#endif /* _SMING_CORE_HTTPSERVER_H_ */
