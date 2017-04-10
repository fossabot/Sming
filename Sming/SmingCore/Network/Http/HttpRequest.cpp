/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 *
 * HttpRequest
 *
 * @author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#include "HttpRequest.h"

HttpRequest::HttpRequest(URL uri) {
	this->uri = uri;
}

HttpRequest::HttpRequest(const HttpRequest& value) {
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

HttpRequest& HttpRequest::operator = (const HttpRequest& rhs) {
	if (this == &rhs) return *this;

	// TODO: FIX this...
//	if (rhs.buffer) copy(rhs.buffer, rhs.len);
//	else invalidate();

	return *this;
}

HttpRequest::~HttpRequest() {

}

HttpRequest* HttpRequest::setURL(URL uri) {
	this->uri = uri;
	return this;
}

HttpRequest* HttpRequest::setMethod(const HttpMethod method)
{
	this->method = method;
	return this;
}

HttpRequest* HttpRequest::setHeaders(const HttpHeaders& headers) {
	for(int i=0; i < headers.count(); i++) {
		this->requestHeaders[headers.keyAt(i)] = headers.valueAt(i);
	}
	return this;
}

HttpRequest* HttpRequest::setHeader(const String& name, const String& value) {
	this->requestHeaders[name] = value; // TODO: add here name and/or value escaping.
	return this;
}


HttpRequest* HttpRequest::setPostParameters(const HttpParams& params)
{
	postParams = params;
	return this;
}

HttpRequest* HttpRequest::setPostParameter(const String& name, const String& value)
{
	postParams[name] = value;
	return this;
}

#ifdef ENABLE_HTTP_REQUEST_AUTH
HttpRequest* HttpRequest::setAuth(AuthAdapter *adapter) {
	adapter->setRequest(this);
	auth = adapter;
	return this;
}
#endif

String HttpRequest::getHeader(const String& name) {
	if(!requestHeaders.contains(name)) {
		return String("");
	}

	return requestHeaders[name];
}

String HttpRequest::getPostParameter(const String& name) {
	if(!postParams.contains(name)) {
		return String("");
	}

	return postParams[name];
}

String HttpRequest::getQueryParameter(String parameterName, String defaultValue /* = "" */)
{
	String query = uri.Query;

	// TODO: split the query string and process the values...
	if(queryParams.contains(parameterName)) {
		return queryParams[parameterName];
	}

	return defaultValue;
}

String HttpRequest::getBody()
{
	return bodyAsString;
}

HttpRequest* HttpRequest::setResponseStream(IOutputStream *stream) {
	outputStream = stream;
	return this;
}

#ifdef ENABLE_SSL
HttpRequest* HttpRequest::setSslOptions(uint32_t sslOptions) {
	this->sslOptions = sslOptions;
 	return this;
}

uint32_t HttpRequest::getSslOptions() {
 	return sslOptions;
}

HttpRequest* HttpRequest::pinCertificate(SSLFingerprints fingerprints) {
	sslFingerprint = fingerprints;
	return this;
}

HttpRequest* HttpRequest::setSslClientKeyCert(SSLKeyCertPair clientKeyCert) {
	this->sslClientKeyCert = clientKeyCert;
	return this;
}

#endif

HttpRequest* HttpRequest::setBody(const String& body) {
	bodyAsString = body;
	return this;
}

HttpRequest* HttpRequest::setBody(uint8_t *rawData, size_t length) {
	this->rawData = rawData;
	this->rawDataLength = length;
	return this;
}

HttpRequest* HttpRequest::setBody(IDataSourceStream *stream) {
	this->stream = stream;
	return this;
}

HttpRequest* HttpRequest::onBody(RequestBodyDelegate delegateFunction) {
	requestBodyDelegate = delegateFunction;
	return this;
}

HttpRequest* HttpRequest::onHeadersComplete(RequestHeadersCompletedDelegate delegateFunction) {
	this->headersCompletedDelegate = delegateFunction;
	return this;
}

HttpRequest* HttpRequest::onRequestComplete(RequestCompletedDelegate delegateFunction) {
	this->requestCompletedDelegate = delegateFunction;
	return this;
}

#ifndef SMING_RELEASE
String HttpRequest::toString() {
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
