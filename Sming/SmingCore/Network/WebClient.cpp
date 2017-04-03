/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 *
 * Authors: 2017-... Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#include "WebClient.h"
#include "../../Services/WebHelpers/base64.h"

#include "lwip/tcp_impl.h"

// WebRequest

WebRequest::WebRequest(URL uri) {
	this->uri = uri;
}

WebRequest::WebRequest(const WebRequest& value) {
	*this = value;
	method = value.method;
	uri = value.uri;
	if(value.requestHeaders.count()) {
		setHeaders(value.requestHeaders);
	}
	headersCompletedDelegate = value.headersCompletedDelegate;
	requestBodyDelegate = value.requestBodyDelegate;
	requestCompletedDelegate = value.requestCompletedDelegate;

	bodyAsString = value.bodyAsString;
	rawData = value.rawData;
	rawDataLength = value.rawDataLength;

	// Notice: We do not copy streams.

#ifdef ENABLE_SSL
	sslOptions = value.sslOptions;
	sslFingerprint = value.sslFingerprint;
	sslClientKeyCert = value.sslClientKeyCert;
#endif
}

WebRequest& WebRequest::operator = (const WebRequest& rhs) {
	if (this == &rhs) return *this;

	// TODO: FIX this...
//	if (rhs.buffer) copy(rhs.buffer, rhs.len);
//	else invalidate();

	return *this;
}

WebRequest::~WebRequest() {

}

WebRequest* WebRequest::setURL(URL uri) {
	this->uri = uri;
	return this;
}

WebRequest* WebRequest::setMethod(const HttpMethod method) {
	this->method = method;
	return this;
}

WebRequest* WebRequest::setHeaders(const Headers& headers) {
	for(int i=0; i < headers.count(); i++) {
		this->requestHeaders[headers.keyAt(i)] = headers.valueAt(i);
	}
	return this;
}

WebRequest* WebRequest::setHeader(const String& name, const String& value) {
	this->requestHeaders[name] = value; // TODO: add here name and/or value escaping.
	return this;
}

WebRequest* WebRequest::setAuth(AuthAdapter *adapter) {
	adapter->setRequest(this);
	auth = adapter;
	return this;
}

WebRequest* WebRequest::setResponseStream(IOutputStream *stream) {
	outputStream = stream;
	return this;
}

#ifdef ENABLE_SSL
WebRequest* WebRequest::setSslOptions(uint32_t sslOptions) {
	this->sslOptions = sslOptions;
 	return this;
}

uint32_t WebRequest::getSslOptions() {
 	return sslOptions;
}

WebRequest* WebRequest::pinCertificate(SSLFingerprints fingerprints) {
	sslFingerprint = fingerprints;
	return this;
}

WebRequest* WebRequest::setSslClientKeyCert(SSLKeyCertPair clientKeyCert) {
	this->sslClientKeyCert = clientKeyCert;
	return this;
}

#endif

WebRequest* WebRequest::setBody(const String& body) {
	bodyAsString = body;
	return this;
}

WebRequest* WebRequest::setBody(uint8_t *rawData, size_t length) {
	this->rawData = rawData;
	this->rawDataLength = length;
	return this;
}

WebRequest* WebRequest::setBody(IDataSourceStream *stream) {
	this->stream = stream;
	return this;
}

WebRequest* WebRequest::onBody(RequestBodyDelegate delegateFunction) {
	requestBodyDelegate = delegateFunction;
	return this;
}

WebRequest* WebRequest::onHeadersComplete(RequestHeadersCompletedDelegate delegateFunction) {
	this->headersCompletedDelegate = delegateFunction;
	return this;
}

WebRequest* WebRequest::onRequestComplete(RequestCompletedDelegate delegateFunction) {
	this->requestCompletedDelegate = delegateFunction;
	return this;
}

#ifndef SMING_RELEASE
String WebRequest::toString() {
	String content = "";
#ifdef ENABLE_SSL
	content += "> SSL options: " + String(sslOptions) + "\n";
	content += "> SSL Cert Fingerprint Length: " + String((sslFingerprint.certSha1 == NULL)? 0: SHA1_SIZE) + "\n";
	content += "> SSL PK Fingerprint Length: " + String((sslFingerprint.pkSha256 == NULL)? 0: SHA256_SIZE) + "\n";
	content += "> SSL ClientCert Length: " + String(sslClientKeyCert.certificateLength) + "\n";
	content += "> SSL ClientCert PK Length: " + String(sslClientKeyCert.keyLength) + "\n";
	content += "\n";
#endif

	content += http_method_str(method) + String(" ") + uri.getPathWithQuery() + " HTTP/1.1\n";
	content += "Host: " + uri.Host + ":" + uri.Port + "\n";
	for(int i=0; i< requestHeaders.count(); i++) {
		content += requestHeaders.keyAt(i) + ": " + requestHeaders.valueAt(i) + "\n";
	}

	if(rawDataLength) {
		content += "Content-Length: " + String(rawDataLength);
	}

	return content;
}
#endif

// Authentication

HttpBasicAuth::HttpBasicAuth(const String& username, const String& password) {
	this->username = username;
	this->password = password;
}

// Basic Auth
void HttpBasicAuth::setRequest(WebRequest* request) {
	String clearText = username+":" + password;
	int hashLength = clearText.length() * 4;
	char hash[hashLength];
	base64_encode(clearText.length(), (const unsigned char *)clearText.c_str(), hashLength, hash);

	request->setHeader("Authorization", "Basic "+ String(hash));
}

// Digest Auth
HttpDigestAuth::HttpDigestAuth(const String& username, const String& password) {
	this->username = username;
	this->password = password;
}

void HttpDigestAuth::setRequest(WebRequest* request) {
	this->request = request;
}

void HttpDigestAuth::setResponse(WebResponse *response) {
	if(response->code != HTTP_STATUS_UNAUTHORIZED) {
		return;
	}

	if(response->headers.contains("WWW-Authenticate") && response->headers["WWW-Authenticate"].indexOf("Digest")!=-1) {
		String authHeader = response->headers["WWW-Authenticate"];
		/*
		 * Example (see: https://tools.ietf.org/html/rfc2069#page-4):
		 *
		 * WWW-Authenticate: Digest    realm="testrealm@host.com",
                            nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
                            opaque="5ccc069c403ebaf9f0171e9517f40e41"
		 *
		 */

		// TODO: process WWW-Authenticate header

		String authResponse = "Digest username=\"" + username + "\"";
		/*
		 * Example (see: https://tools.ietf.org/html/rfc2069#page-4):
		 *
		 * Authorization: Digest       username="Mufasa",
                            realm="testrealm@host.com",
                            nonce="dcd98b7102dd2f0e8b11d0f600bfb0c093",
                            uri="/dir/index.html",
                            response="e966c932a9242554e42c8ee200cec7f6",
                            opaque="5ccc069c403ebaf9f0171e9517f40e41"
		 */

		// TODO: calculate the response...
		request->setHeader("Authorization", authResponse);
		request->retries = 1;
	}
}

// HttpConnection
HttpConnection::HttpConnection(RequestQueue* queue): TcpClient(false), mode(eHCM_String) {
	this->waitingQueue = queue;
}

bool HttpConnection::connect(const String& host, int port, bool useSsl /* = false */, uint32_t sslOptions /* = 0 */) {

	debugf("HttpConnection::connect: TCP state: %d, isStarted: %d, isActive: %d", (tcp != NULL? tcp->state : -1), (int)(getConnectionState() != eTCS_Ready), (int)isActive());

	if(isProcessing()) {
		return true;
	}

	if(getConnectionState() != eTCS_Ready && isActive()) {
		debugf("HttpConnection::reusing TCP connection ");

		// we might have still alive connection
		onConnected(ERR_OK);
		return true;
	}

	debugf("HttpConnection::connecting ...");

	return TcpClient::connect(host, port, useSsl, sslOptions);
}

bool HttpConnection::isActive() {
	if(tcp == NULL) {
		return false;
	}

	struct tcp_pcb *pcb;
	for(pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
		if(tcp == pcb) {
			return true;
		}
	}

	return false;
}

// @deprecated
HashMap<String, String> &HttpConnection::getResponseHeaders()
{
	return responseHeaders;
}

String HttpConnection::getResponseHeader(String headerName, String defaultValue /* = "" */)
{
	if (responseHeaders.contains(headerName))
		return responseHeaders[headerName];

	return defaultValue;
}

DateTime HttpConnection::getLastModifiedDate()
{
	DateTime res;
	String strLM = getResponseHeader("Last-Modified");
	if (res.parseHttpDate(strLM))
		return res;
	else
		return DateTime();
}

DateTime HttpConnection::getServerDate()
{
	DateTime res;
	String strSD = getResponseHeader("Date");
	if (res.parseHttpDate(strSD))
		return res;
	else
		return DateTime();
}

String HttpConnection::getResponseString()
{
	if (mode == eHCM_String)
		return responseStringData;
	else
		return "";
}

// @enddeprecated

void HttpConnection::reset()
{
	if(currentRequest != NULL) {
		delete currentRequest;
		currentRequest = NULL;
	}

	code = 0;
	responseStringData = "";
	responseHeaders.clear();

	lastWasValue = true;
	lastData = "";
	currentField  = "";
}


err_t HttpConnection::onProtocolUpgrade(http_parser* parser)
{
	debugf("onProtocolUpgrade: Protocol upgrade is not supported");
	return ERR_ABRT;
}

int HttpConnection::staticOnMessageBegin(http_parser* parser)
{
	HttpConnection *connection = (HttpConnection*)parser->data;
	if(connection == NULL) {
		// something went wrong
		return -1;
	}

	connection->reset();

	connection->currentRequest = connection->executionQueue.dequeue();
	if(connection->currentRequest == NULL) {
		return 1; // there are no requests in the queue
	}

	if(connection->currentRequest->outputStream != NULL) {
		connection->mode = eHCM_Stream;
	}
	else {
		connection->mode = eHCM_String;
	}

	return 0;
}

int HttpConnection::staticOnMessageComplete(http_parser* parser)
{
	HttpConnection *connection = (HttpConnection*)parser->data;
	if(connection == NULL) {
		// something went wrong
		return -1;
	}

	if(!connection->currentRequest) {
		return -2; // no current request...
	}

	debugf("staticOnMessageComplete: Execution queue: %d, %s",
								connection->executionQueue.count(),
								connection->currentRequest->uri.toString().c_str()
								);

	// we are finished with this request
	int hasError = 0;
	if(connection->currentRequest->requestCompletedDelegate) {
		bool success = (HTTP_PARSER_ERRNO(parser) == HPE_OK) &&  // false when the parsing has failed
					   (connection->code >= 200 && connection->code <= 399);  // false when the HTTP status code is not ok
		hasError = connection->currentRequest->requestCompletedDelegate(*connection, success);
	}

	if(connection->currentRequest->auth != NULL) {
		connection->currentRequest->auth->setResponse(connection->getResponse());
	}

	if(connection->currentRequest->retries > 0) {
		connection->currentRequest->retries--;
		return (connection->executionQueue.enqueue(connection->currentRequest)? 0: -1);
	}

	if(connection->currentRequest->outputStream != NULL) {
		connection->currentRequest->outputStream->close();
		delete connection->currentRequest->outputStream;
	}

	delete connection->currentRequest;
	connection->currentRequest = NULL;

	if(!connection->executionQueue.count()) {
		connection->onConnected(ERR_OK);
	}

	return hasError;
}

int HttpConnection::staticOnHeadersComplete(http_parser* parser)
{
	HttpConnection *connection = (HttpConnection*)parser->data;
	if(connection == NULL) {
		// something went wrong
		return -1;
	}

	debugf("The headers are complete");

	/* Callbacks should return non-zero to indicate an error. The parser will
	 * then halt execution.
	 *
	 * The one exception is on_headers_complete. In a HTTP_RESPONSE parser
	 * returning '1' from on_headers_complete will tell the parser that it
	 * should not expect a body. This is used when receiving a response to a
	 * HEAD request which may contain 'Content-Length' or 'Transfer-Encoding:
	 * chunked' headers that indicate the presence of a body.
	 *
	 * Returning `2` from on_headers_complete will tell parser that it should not
	 * expect neither a body nor any futher responses on this connection. This is
	 * useful for handling responses to a CONNECT request which may not contain
	 * `Upgrade` or `Connection: upgrade` headers.
	 */

	connection->code = parser->status_code;
	if(connection->currentRequest == NULL) {
		// nothing to process right now...
		return 1;
	}

	int error = 0;
	if(connection->currentRequest->headersCompletedDelegate) {
		error = connection->currentRequest->headersCompletedDelegate(*connection, connection->responseHeaders);
	}

	if(!error && connection->currentRequest->method == HTTP_HEAD) {
		error = 1;
	}

	return error;
}

int HttpConnection::staticOnStatus(http_parser *parser, const char *at, size_t length) {
	return 0;
}

int HttpConnection::staticOnHeaderField(http_parser *parser, const char *at, size_t length)
{
	HttpConnection *connection = (HttpConnection*)parser->data;
	if(connection == NULL) {
		// something went wrong
		return -1;
	}

	if(connection->lastWasValue) {
		// we are starting to process new header
		connection->lastData = "";
		connection->lastWasValue = false;
	}
	connection->lastData += String(at, length);

	return 0;
}

int HttpConnection::staticOnHeaderValue(http_parser *parser, const char *at, size_t length)
{
	HttpConnection *connection = (HttpConnection*)parser->data;
	if (connection == NULL) {
		// something went wrong
		return -1;
	}

	if(!connection->lastWasValue) {
		connection->currentField = connection->lastData;
		connection->responseHeaders[connection->currentField] = "";
		connection->lastWasValue = true;
	}
	connection->responseHeaders[connection->currentField] += String(at, length);

	return 0;
}

int HttpConnection::staticOnBody(http_parser *parser, const char *at, size_t length)
{
	HttpConnection *connection = (HttpConnection*)parser->data;
	if (connection == NULL) {
		// something went wrong
		return -1;
	}

	if(connection->currentRequest->requestBodyDelegate) {
		return connection->currentRequest->requestBodyDelegate(*connection, at, length);
	}

	if (connection->mode == eHCM_String) {
		connection->responseStringData += String(at, length);
		return 0;
	}

	if(connection->currentRequest->outputStream != NULL) {
		int res = connection->currentRequest->outputStream->write((const uint8_t *)&at, length);
		if (res != length) {
			connection->currentRequest->outputStream->close();
			return 1;
		}
	}

	return 0;
}

int HttpConnection::staticOnChunkHeader(http_parser* parser) {
	debugf("On chunk header");
	return 0;
}

int HttpConnection::staticOnChunkComplete(http_parser* parser) {
	debugf("On chunk complete");
	return 0;
}

err_t HttpConnection::onConnected(err_t err) {
	if (err == ERR_OK) {
		// create parser ...
		if(parser == NULL) {
			parser = new http_parser;
			http_parser_init(parser, HTTP_RESPONSE);
			parser->data = (void*)this;

			memset(&parserSettings, 0, sizeof(parserSettings));

			// Notification callbacks: on_message_begin, on_headers_complete, on_message_complete.
			parserSettings.on_message_begin     = staticOnMessageBegin;
			parserSettings.on_headers_complete  = staticOnHeadersComplete;
			parserSettings.on_message_complete  = staticOnMessageComplete;

			parserSettings.on_chunk_header   = staticOnChunkHeader;
			parserSettings.on_chunk_complete = staticOnChunkComplete;


			// Data callbacks: on_url, (common) on_header_field, on_header_value, on_body;
			parserSettings.on_status            = staticOnStatus;
			parserSettings.on_header_field      = staticOnHeaderField;
			parserSettings.on_header_value      = staticOnHeaderValue;
			parserSettings.on_body              = staticOnBody;
		}

		debugf("HttpConnection::onConnected: waitingQueue.count: %d", waitingQueue->count());

		do {
			WebRequest* request = waitingQueue->peek();
			if(request == NULL) {
				break;
			}

			if(!executionQueue.enqueue(request)) {
				debugf("The working queue is full at the moment");
				break;
			}

			waitingQueue->dequeue();
			send(request);

			if(!(request->method == HTTP_GET || request->method == HTTP_HEAD)) {
				// if the current request cannot be pipelined -> break;
				break;
			}

			WebRequest* nextRequest = waitingQueue->peek();
			if(nextRequest != NULL && !(nextRequest->method == HTTP_GET || nextRequest->method == HTTP_HEAD))  {
				// if the next request cannot be pipelined -> break for now
				break;
			}
		} while(1);
	}

	TcpClient::onConnected(err);
	return ERR_OK;
}

void HttpConnection::send(WebRequest* request) {
	sendString(http_method_str(request->method) + String(" ") + request->uri.getPathWithQuery() + " HTTP/1.1\r\nHost: " + request->uri.Host + "\r\n");

	// take care to adjust the content-length
	if(request->rawDataLength) {
		request->requestHeaders["Content-Length"] = String(request->rawDataLength);
	}
	else if(request->bodyAsString.length()) {
		request->requestHeaders["Content-Length"] = String(request->bodyAsString.length());
	}
	else if (request->stream != NULL ) {
		// TODO:: if the stream has a size -> use it otherwise use chunked encoding
		if(request->requestHeaders.contains("Content-Length")) {
			request->requestHeaders.remove("Content-Length");
		}
		request->requestHeaders["Transfer-Encoding"] = "chunked";
	}
	else {
		request->requestHeaders["Content-Length"] = "0";
	}

	for (int i = 0; i < request->requestHeaders.count(); i++)
	{
		String write = request->requestHeaders.keyAt(i) + ": " + request->requestHeaders.valueAt(i) + "\r\n";
		sendString(write.c_str());
	}
	sendString("\r\n");

	// if there is input raw data -> send it
	if(request->rawDataLength > 0) {
		TcpClient::send((const char*)request->rawData, (uint16_t)request->rawDataLength);
	}
	if(request->stream != NULL) {
		send(request->stream);

		debugf("Stream completed");
		delete request->stream;
		request->stream = NULL;
	}
	else if(request->bodyAsString.length()) {
		sendString(request->bodyAsString);
	}
}

WebRequest* HttpConnection::getRequest() {
	return currentRequest;
}

WebResponse* HttpConnection::getResponse() {
	WebResponse* response = new WebResponse();
	response->code = code;
	response->headers = responseHeaders;
	if(currentRequest) {
		response->stream = currentRequest->outputStream;
	}
	response->bodyAsString = responseStringData;
}

// end of public methods for HttpConnection

err_t HttpConnection::onReceive(pbuf *buf) {
	if (buf == NULL)
	{
		// Disconnected, close it
		return TcpClient::onReceive(buf);
	}

	pbuf *cur = buf;
	int parsedBytes = 0;
	while (cur != NULL && cur->len > 0) {
		parsedBytes += http_parser_execute(parser, &parserSettings, (char*) cur->payload, cur->len);
		if(HTTP_PARSER_ERRNO(parser) != HPE_OK) {
			// we ran into trouble - abort the connection
			debugf("HTTP parser error: %s", http_errno_name(HTTP_PARSER_ERRNO(parser)));
			cleanup();
			TcpClient::onReceive(NULL);
			return ERR_ABRT;
		}

		cur = cur->next;
	}

	if (parser->upgrade) {
		return onProtocolUpgrade(parser);
	} else if (parsedBytes != buf->tot_len) {
		TcpClient::onReceive(NULL);

		return ERR_ABRT;
	}

	// Fire ReadyToSend callback
	TcpClient::onReceive(buf);

	return ERR_OK;
}

void HttpConnection::onError(err_t err) {
	cleanup();
	TcpClient::onError(err);
}

void HttpConnection::cleanup() {
	// TODO: clean the current request
	reset();

	// TODO: clean the current response

	// if there are requests in the executionQueue -> move them back to the waiting queue
	for(int i=0; i < executionQueue.count(); i++) {
		waitingQueue->enqueue(executionQueue.dequeue());
	}

	if(parser != NULL) {
		delete parser;
		parser = NULL;
	}
}

HttpConnection::~HttpConnection() {
	cleanup();
}

// WebClient

/* Low Level Methods */
bool HttpClient::send(WebRequest* request) {
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

	// if that is old httpConnection object from another httpClient -> reuse it and add the new queue.
	// TODO: check if that is working as expected...
//	httpConnectionPool[cacheKey]->waitingQueue = queue[cacheKey];

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


WebRequest* HttpClient::request(const String& url) {
	return new WebRequest(URL(url));
}

HashMap<String, HttpConnection *> HttpClient::httpConnectionPool;
HashMap<String, RequestQueue* > HttpClient::queue;

// TODO:: Free connection pool

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
