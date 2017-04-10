/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 *
 * HttpResponse
 *
 * @author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef _SMING_CORE_HTTP_RESPONSE_H_
#define _SMING_CORE_HTTP_RESPONSE_H_

#include "HttpCommon.h"
#include "../../OutputStream.h"
#include "../../DataSourceStream.h"

class JsonObjectStream; // TODO: fix this later...

class HttpResponse {
	friend class HttpClient;
	friend class HttpConnection;
	friend class HttpServerConnection;

public:

	bool sendString(const String& text);

	// @deprecated method

	bool hasHeader(const String name);

	void redirect(const String& location);
	__forceinline void forbidden() {
		code = HTTP_STATUS_FORBIDDEN;
	}

	HttpResponse* setContentType(const String type);
	HttpResponse* setContentType(enum MimeType type);
	HttpResponse* setCookie(const String name, const String value);
	HttpResponse* setHeader(const String name, const String value);
	HttpResponse* setCache(int maxAgeSeconds = 3600, bool isPublic = false);
	HttpResponse* setAllowCrossDomainOrigin(String controlAllowOrigin); // Access-Control-Allow-Origin for AJAX from a different domain

	// Send file by name
	bool sendFile(String fileName, bool allowGzipFileCheck = true);

	// Parse and send template file
	bool sendTemplate(TemplateFileStream* newTemplateInstance);

	// Build and send JSON string
	bool sendJsonObject(JsonObjectStream* newJsonStreamInstance);
	// Send Datastream, can be called with Classes derived from
	bool sendDataStream( IDataSourceStream * newDataStream , String reqContentType = "" );

	// @end deprecated


public:
	int code;
	HttpHeaders headers;
	String bodyAsString;
	IDataSourceStream* stream = NULL;
};

#endif /* _SMING_CORE_HTTP_RESPONSE_H_ */
