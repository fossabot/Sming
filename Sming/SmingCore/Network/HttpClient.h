/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * http://github.com/SmingHub/Sming
 *
 * Author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef _SMING_CORE_NETWORK_HTTPCLIENT_H_
#define _SMING_CORE_NETWORK_HTTPCLIENT_H_

#include "TcpClient.h"
#include "Http/HttpCommon.h"
#include "Http/HttpRequest.h"
#include "Http/HttpConnection.h"

class HttpClient
{
public:
	/* High-Level Method */

	__forceinline bool sendRequest(const String& url, RequestCompletedDelegate requestComplete) {
		return send(request(url)
				   ->setMethod(HTTP_GET)
				   ->onRequestComplete(requestComplete)
				   );
	}


	__forceinline bool sendRequest(const HttpMethod method, const String& url, const HttpHeaders& headers, RequestCompletedDelegate requestComplete) {
		return send(request(url)
				   ->setMethod(HTTP_GET)
				   ->setHeaders(headers)
				   ->onRequestComplete(requestComplete)
				   );
	}

	__forceinline bool sendRequest(const HttpMethod method, const String& url, const HttpHeaders& headers, const String& body, RequestCompletedDelegate requestComplete) {
			return send(request(url)
					   ->setMethod(method)
					   ->setHeaders(headers)
					   ->setBody(body)
					   ->onRequestComplete(requestComplete)
					   );
	}

	bool downloadString(const String& url, RequestCompletedDelegate requestComplete);

	__forceinline bool downloadFile(const String& url, RequestCompletedDelegate requestComplete = NULL) {
		return downloadFile(url, "", requestComplete);
	}

	bool downloadFile(const String& url, const String& saveFileName, RequestCompletedDelegate requestComplete = NULL);

	/* Low Level Methods */
	bool send(HttpRequest* request);
	HttpRequest* request(const String& url);

#ifdef ENABLE_SSL
	static void freeSslSessionPool();
#endif

	/**
	 * Use this method to clean all request queues and object pools
	 */
	static void cleanup();

	virtual ~HttpClient();

protected:
	String getCacheKey(URL url);

protected:
	static HashMap<String, HttpConnection *> httpConnectionPool;
	static HashMap<String, RequestQueue* > queue;

#ifdef ENABLE_SSL
	static HashMap<String, SSLSessionId* > sslSessionIdPool;
#endif

};

/**

@code

HttpClient client = new HttpClient();
Headers requestHeaders;

client->sendRequest("https://attachix.com/img/a.gif", onRequestComplete);
client->sendRequest("https://attachix.com/img/b.gif", onRequestComplete);
client->sendRequest("https://attachix.com/css/c.css", onRequestComplete);
client->sendRequest("https://attachix.com/js/d.js", onRequestComplete);
client->sendRequest("https://attachix.com/js/e.js", onRequestComplete);

// push all in queue. start connection to the remote server...
// if onConnected is called -> if 1,2..N are get requests to the same server, then


client->sendRequest(HTTP_GET, "https://attachix.com/update/check", requestHeaders, NULL, [](HttpClient& client, bool successful) -> void {
	if(!successful) {
		// TODO:
		return;
	}
});

@endcode

*/


#endif /* _SMING_CORE_NETWORK_HTTPCLIENT_H_ */
