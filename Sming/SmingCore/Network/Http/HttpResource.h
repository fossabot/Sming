/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 *
 * HttpResource
 *
 * @author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 * All files of the Sming Core are provided under the LGPL v3 license.
 ****/

#ifndef _SMING_CORE_HTTP_RESOURCE_H_
#define _SMING_CORE_HTTP_RESOURCE_H_

#include "../../Wiring/WString.h"
#include "../../Wiring/WHashMap.h"
#include "../../Delegate.h"

#include "HttpResponse.h"
#include "HttpRequest.h"

class HttpServerConnection;

typedef Delegate<int(HttpServerConnection& connection, HttpRequest&, const char *at, int length)> HttpServerConnectionBodyDelegate;
typedef Delegate<int(HttpServerConnection&, HttpRequest&, HttpResponse&)> HttpResourceDelegate;

class HttpResource {
public:
	virtual ~HttpResource() {}

public:
	HttpServerConnectionBodyDelegate onBody = 0;
	HttpResourceDelegate onHeadersComplete = 0;
	HttpResourceDelegate onRequestComplete = 0;
};

typedef HashMap<String, HttpResource> ResourceTree;

#endif /* _SMING_CORE_HTTP_RESOURCE_H_ */
