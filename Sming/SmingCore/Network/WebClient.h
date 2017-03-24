/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * http://github.com/SmingHub/Sming
 *
 * Author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef _SMING_CORE_NETWORK_WEBCLIENT_H_
#define _SMING_CORE_NETWORK_WEBCLIENT_H_

#include "TcpClient.h"
#include "../../Wiring/WString.h"
#include "../../Wiring/WHashMap.h"
#include "../../Wiring/FILO.h"
#include "../../Services/DateTime/DateTime.h"
#include "../SmingCore/Delegate.h"
#include "URL.h"
#include "../SmingCore/DataSourceStream.h"
#include "../SmingCore/OutputStream.h"

#define HTTP_MAX_HEADER_SIZE  (8*1024)

#include "../http-parser/http_parser.h"

typedef HashMap<String, String> WebClientParams;

/* Number of maximum tcp connections to be kept in the pool */
#define REQUEST_POOL_SIZE 20

typedef enum http_method HttpMethod;

template<typename T, int rawSize>
class SimpleQueue: public FIFO<T, rawSize> {
	virtual const T& operator[](unsigned int) const { }
	virtual T& operator[](unsigned int) { }
};


enum WebClientMode
{
	eWCM_String = 0,
	eWCM_File, // << Deprecated! Use eWCM_Stream stream instead
	eWCM_Stream,
	eWCM_UserDefined
};

/**
 * Ways to speed up a web client
 * 1 ) Pipeline all consecutive GET requests and push them as ONE request
 * Example:
 * GET http://host:port/a/b
 * GET http://host:port/b/d
 * GET http://host:port/c/f
 * POST http://host:port/c/f
 * GET http://host:port/c/f
 *
 * should result in one TCP request with 3 GET requests in it, followed by one POST and one final GET request.
 *
 * 2 ) Persistent Connections: https://en.wikipedia.org/wiki/HTTP_persistent_connection
 *
 * 3 ) SSL session id re-usage: A session id should be valid for 30 minutes or even an hour. (Most CPU intensive -> no-need for additional handshake)
 *
 *
 * webClient=new WebClient();
 *
 * High-Level
 * webClient->sendRequest(url, onRequestComplete) ...
 * webClient->sendRequest(method, url, const String&, onRequestComplete) ...
 *
 * Low Level
 * webClient->send(
 * 			webClient->request(url)
 * 			->setMethod(HttpMethod::GET)
 * 			->setHeaders()
 *
 * 			->setCookies(...)
 * 			->setAuth(Adapter...)
 *
 * 			->setBody(const String&)
 * 			->setBody(uint8_t *rawData)
 * 			->setBody(IDataStream *stream)
 *
 * 			->onHeadersComplete(typedef <bool<http_parser* parser, HashMap<String,String> headers> RequestHeadersCompletedDelegate)
 * 			->onRequestComplete(http_parser* parser)
 * 			)
 *

 *
 *
 */

class HttpConnection;

typedef HashMap<String, String> Headers;

typedef Delegate<int(HttpConnection& client, Headers& headers)> RequestHeadersCompletedDelegate;
typedef Delegate<int(HttpConnection& client, const char *at, size_t length)> RequestBodyDelegate;
typedef Delegate<void(HttpConnection& client, bool successful)> RequestCompletedDelegate;

class WebResponse {
	friend class WebClient;
	friend class HttpConnection;

public:
	int code;
	Headers headers;
	IOutputStream *stream = NULL;
	String bodyAsString;
};

class WebRequest {
	friend class WebClient;
	friend class HttpConnection;

public:

	WebRequest(URL uri);

	WebRequest* setURL(URL uri);

	WebRequest* setMethod(const HttpMethod method);

	WebRequest* setHeaders(const Headers& headers);

//	TODO: here we can add authentication adapters, if needed
//	WebRequest* setAuth(Adapter...)

#ifdef ENABLE_SSL
 	WebRequest* setSslOptions(uint32_t sslOptions);
 	uint32_t getSslOptions();
 	WebRequest* pinCertificate(const uint8_t *fingerprint, SslFingerprintType type);
#endif

	WebRequest* setBody(const String& body);
	WebRequest* setBody(IDataSourceStream *stream);
	WebRequest* setBody(uint8_t *rawData, size_t length);

	WebRequest* setResponseStream(IOutputStream *stream);

	WebRequest* onHeadersComplete(RequestHeadersCompletedDelegate delegateFunction);
	WebRequest* onBody(RequestBodyDelegate delegateFunction);
	WebRequest* onRequestComplete(RequestCompletedDelegate delegateFunction);


public:
	URL uri;
	HttpMethod method = HTTP_GET;
	Headers requestHeaders;

protected:
	RequestHeadersCompletedDelegate headersCompletedDelegate;
	RequestBodyDelegate requestBodyDelegate;
	RequestCompletedDelegate requestCompletedDelegate;
	String bodyAsString;
	uint8_t *rawData = NULL;
	size_t rawDataLength = 0;
	IDataSourceStream *stream=NULL;

	IOutputStream *outputStream=NULL;

#ifdef ENABLE_SSL
	uint32_t sslOptions = 0;
	SSLFingerprints sslFingerprint;
#endif
};


typedef SimpleQueue<WebRequest*, REQUEST_POOL_SIZE> RequestQueue;


class HttpConnection : protected TcpClient {
	friend class WebClient;

public:
	HttpConnection(RequestQueue* queue);
	~HttpConnection();

	bool connect(const String& host, int port, bool useSsl = false, uint32_t sslOptions = 0);

	void send(WebRequest* request);

	/**
	 * @brief Returns pointer to the current request
	 * @return WebRequest*
	 */
	WebRequest* getRequest();

	/**
	 * @brief Returns pointer to the current response
	 * @return WebResponse*
	 */
	WebResponse* getResponse();

	using TcpClient::close;

#ifdef ENABLE_SSL
	using TcpClient::getSsl;
#endif

	// Backported for compatibility reasons
	// @deprecated
	__forceinline int getResponseCode() { return code; }
	String getResponseHeader(String headerName, String defaultValue = "");
	Headers &getResponseHeaders();
	DateTime getLastModifiedDate(); // Last-Modified header
	DateTime getServerDate(); // Date header

	String getResponseString();
	// @enddeprecated



protected:
	void reset();

	virtual err_t onConnected(err_t err);
	virtual err_t onReceive(pbuf *buf);
	virtual err_t onProtocolUpgrade(http_parser* parser);

	virtual void onError(err_t err);

	bool send(IDataSourceStream* inputStream, bool forceCloseAfterSent = false);

	void cleanup();

private:
	static int staticOnMessageBegin(http_parser* parser);
	static int staticOnStatus(http_parser *parser, const char *at, size_t length);
	static int staticOnHeadersComplete(http_parser* parser);
	static int staticOnHeaderField(http_parser *parser, const char *at, size_t length);
	static int staticOnHeaderValue(http_parser *parser, const char *at, size_t length);
	static int staticOnBody(http_parser *parser, const char *at, size_t length);
	static int staticOnChunkHeader(http_parser* parser);
	static int staticOnChunkComplete(http_parser* parser);
	static int staticOnMessageComplete(http_parser* parser);

protected:
	WebClientMode mode;
	String responseStringData;

	RequestQueue* waitingQueue;
	RequestQueue executionQueue;
	http_parser *parser = NULL;
	http_parser_settings parserSettings;
	Headers responseHeaders;

	int code = 0;
	bool lastWasValue = true;
	String lastData = "";
	String currentField  = "";
	WebRequest* currentRequest = NULL;
};

class WebClient
{

public:
	/* High-Level Method */

	__forceinline bool sendRequest(const String& url, RequestCompletedDelegate requestComplete) {
		return send(request(url)
				   ->setMethod(HTTP_GET)
				   ->onRequestComplete(requestComplete)
				   );
	}


	__forceinline bool sendRequest(const HttpMethod method, const String& url, const Headers& headers, RequestCompletedDelegate requestComplete) {
		return send(request(url)
				   ->setMethod(HTTP_GET)
				   ->setHeaders(headers)
				   ->onRequestComplete(requestComplete)
				   );
	}

	__forceinline bool sendRequest(const HttpMethod method, const String& url, const Headers& headers, const String& body, RequestCompletedDelegate requestComplete) {
			return send(request(url)
					   ->setMethod(method)
					   ->setHeaders(headers)
					   ->setBody(body)
					   ->onRequestComplete(requestComplete)
					   );
	}

	bool downloadString(const String& url, RequestCompletedDelegate requestComplete);

	bool downloadFile(String url, String saveFileName, RequestCompletedDelegate requestComplete = NULL);

	/* Low Level Methods */
	bool send(WebRequest* request);
	WebRequest* request(const String& url);

#ifdef ENABLE_SSL
	static void freeSslSessionPool();
#endif

	virtual ~WebClient();

protected:
	String getCacheKey(URL url);

protected:
	HashMap<String, tcp_pcb* > tcpPool;
	HashMap<String, HttpConnection *> httpConnectionPool;
	HashMap<String, RequestQueue* > queue;

#ifdef ENABLE_SSL
	static HashMap<String, SSLSessionId* > sslSessionIdPool;
#endif

};

/**

@code

WebClient client = new WebClient();
Headers requestHeaders;

client->sendRequest("https://attachix.com/img/a.gif", onRequestComplete);
client->sendRequest("https://attachix.com/img/b.gif", onRequestComplete);
client->sendRequest("https://attachix.com/css/c.css", onRequestComplete);
client->sendRequest("https://attachix.com/js/d.js", onRequestComplete);
client->sendRequest("https://attachix.com/js/e.js", onRequestComplete);

// push all in queue. start connection to the remote server...
// if onConnected is called -> if 1,2..N are get requests to the same server, then


client->sendRequest(HTTP_GET, "https://attachix.com/update/check", requestHeaders, NULL, [](WebClient& client, bool successful) -> void {
	if(!successful) {
		// TODO:
		return;
	}
});

@endcode

*/


#endif /* _SMING_CORE_NETWORK_WEBCLIENT_H_ */
