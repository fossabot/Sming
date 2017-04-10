/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 *
 * HttpClient
 *
 * @author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#include "HttpClient.h"

/* Low Level Methods */
bool HttpClient::send(HttpRequest* request) {
	String cacheKey = getCacheKey(request->uri);
	bool useSsl = (request->uri.Protocol == HTTPS_URL_PROTOCOL);

	if(!queue.contains(cacheKey)) {
		queue[cacheKey] = new RequestQueue;
	}

	if(!queue[cacheKey]->enqueue(request)) {
		// the queue is full and we cannot add more requests at the time.
		debugf("The request queue is full at the moment");
		return false;
	}

	if(httpConnectionPool.contains(cacheKey) &&
	   !(httpConnectionPool[cacheKey]->getConnectionState() == eTCS_Ready || httpConnectionPool[cacheKey]->isActive())
	) {
		httpConnectionPool.remove(cacheKey);
	}

	if(!httpConnectionPool.contains(cacheKey)) {
		debugf("Creating new httpConnection");
		httpConnectionPool[cacheKey] = new HttpConnection(queue[cacheKey]);
	}

#ifdef ENABLE_SSL
	// Based on the URL decide if we should reuse the SSL and TCP pool
	if(useSsl) {
		if (!sslSessionIdPool.contains(cacheKey)) {
			sslSessionIdPool[cacheKey] = (SSLSessionId *)malloc(sizeof(SSLSessionId));
			sslSessionIdPool[cacheKey]->value = NULL;
			sslSessionIdPool[cacheKey]->length = 0;
		}
		httpConnectionPool[cacheKey]->addSslOptions(request->getSslOptions());
		httpConnectionPool[cacheKey]->pinCertificate(request->sslFingerprint);
		httpConnectionPool[cacheKey]->setSslClientKeyCert(request->sslClientKeyCert);
		httpConnectionPool[cacheKey]->sslSessionId = sslSessionIdPool[cacheKey];
	}
#endif

	return httpConnectionPool[cacheKey]->connect(request->uri.Host, request->uri.Port, useSsl);
}

// @deprecated

bool HttpClient::downloadString(const String& url, RequestCompletedDelegate requestComplete) {
	return send(request(url)
				->setMethod(HTTP_GET)
				->onRequestComplete(requestComplete)
				);
}

bool HttpClient::downloadFile(const String& url, const String& saveFileName, RequestCompletedDelegate requestComplete /* = NULL */)
{
	URL uri = URL(url);

	String file;
	if (saveFileName.length() == 0)
	{
		file = uri.Path;
		int p = file.lastIndexOf('/');
		if (p != -1)
			file = file.substring(p + 1);
	}
	else
		file = saveFileName;

	FileOutputStream* fileStream = new FileOutputStream(saveFileName);

	return send(request(url)
				   ->setResponseStream(fileStream)
				   ->setMethod(HTTP_GET)
				   ->onRequestComplete(requestComplete)
			  );
}

// @enddeprecated

// HttpConnection ...

bool HttpConnection::send(IDataSourceStream* inputStream, bool forceCloseAfterSent /* = false*/)
{
	do {
		int len = 256;
		char data[len];
		len = inputStream->readMemoryBlock(data, len);

		// send the data in chunks...
		sendString(String(len)+ "\r\n");
		TcpClient::send(data, len);
		sendString("\n\r");
		inputStream->seek(max(len, 0));
	} while(!inputStream->isFinished());

	sendString("0\r\n\r\n", forceCloseAfterSent);

	return true;
}


HttpRequest* HttpClient::request(const String& url) {
	return new HttpRequest(URL(url));
}

HashMap<String, HttpConnection *> HttpClient::httpConnectionPool;
HashMap<String, RequestQueue* > HttpClient::queue;

#ifdef ENABLE_SSL
HashMap<String, SSLSessionId* > HttpClient::sslSessionIdPool;

void HttpClient::freeSslSessionPool() {
	for(int i=0; i< sslSessionIdPool.count(); i ++) {
		String key = sslSessionIdPool.keyAt(i);
		if(sslSessionIdPool[key]->value != NULL) {
			free(sslSessionIdPool[key]->value);
		}
		free(sslSessionIdPool[key]->value);
	}
	sslSessionIdPool.clear();
}
#endif

void HttpClient::cleanup() {
#ifdef ENABLE_SSL
	freeSslSessionPool();
#endif
	httpConnectionPool.clear();
	queue.clear();
}


HttpClient::~HttpClient() {

}

String HttpClient::getCacheKey(URL url) {
	return String(url.Host) + ":" + String(url.Port);
}
