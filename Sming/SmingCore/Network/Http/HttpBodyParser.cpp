/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/anakod/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * HttpBodyParser
 *
 * @author: 2017 - Slavey Karadzhov <slav@attachix.com>
 *
 ****/

#include "HttpBodyParser.h"
#include "../WebHelpers/escape.h"

#ifdef ENABLE_HTTP_SERVER_MULTIPART
#include "../third-party/multipart-parser/multipart_parser.h"


class MultiPartParser {

public:
	MultiPartParser(HttpRequest* request);
	~MultiPartParser();

	void execute(const char *at, size_t length);

	static int readHeaderName(multipart_parser_t* p, const char *at, size_t length);
	static int readHeaderValue(multipart_parser_t* p, const char *at, size_t length);
	static int partBegin(multipart_parser_t* p);
	static int partData(multipart_parser_t* p, const char *at, size_t length);
	static int partEnd(multipart_parser_t* p);
	static int bodyEnd(multipart_parser_t* p);

private:
	multipart_parser_settings_t settings;

	HttpParams params;
	bool useValue = false;

	HttpRequest* request;

	multipart_parser_t* parser;
	String name; // current parameter name
};

MultiPartParser::MultiPartParser(HttpRequest* request)
{
	memset(&settings, 0, sizeof(settings));
	settings.on_header_field = readHeaderName;
	settings.on_header_value = readHeaderValue;
	settings.on_part_data_begin = partBegin;
	settings.on_part_data = partData;
	settings.on_part_data_end = partEnd;
	settings.on_body_end = bodyEnd;

	if(request->headers.contains("Content-Type")) {
		// Content-Type: multipart/form-data; boundary=------------------------a48863c0572edce6
		int startPost = request->headers["Content-Type"].indexOf("boundary=");
		if(startPost == -1) {
			return;
		}

		startPost += 9;
		String boundary = "--" + request->headers["Content-Type"].substring(startPost);
		parser = multipart_parser_init(boundary.c_str(), &settings);
	}

	this->request = request;
	parser->data = this;
}

MultiPartParser::~MultiPartParser()
{
	if(parser != NULL) {
		multipart_parser_free(parser);
	}
}

void MultiPartParser::execute(const char *at, size_t length)
{
	multipart_parser_execute(parser, at, length);
}

int MultiPartParser::partBegin(multipart_parser_t* p)
{
	MultiPartParser* parser = (MultiPartParser *)p->data;
	if(parser == NULL) {
		return -1;
	}

	parser->useValue = false;
	return 0;
}

int MultiPartParser::readHeaderName(multipart_parser_t* p, const char *at, size_t length)
{
   MultiPartParser* parser = (MultiPartParser *)p->data;
   if(parser == NULL) {
	   return -1;
   }

   if(memcmp(at, "Content-Disposition", length) == 0) {
	   parser->useValue = true;
   }

   return 0;
}

int MultiPartParser::readHeaderValue(multipart_parser_t* p, const char *at, size_t length)
{
   MultiPartParser* parser = (MultiPartParser *)p->data;
   if(parser == NULL) {
	   return -1;
   }

   if(parser->useValue) {
	   // Content-Disposition: form-data; name="image"; filename=".gitignore"
	   // Content-Disposition: form-data; name="data"
	   String value = String(at, length);
	   int startPos = value.indexOf("name=");
	   if(startPos == -1) {
		   return -1; // Invalid header content
	   }
	   startPos += 6; // name="
	   int endPos = value.indexOf(';', startPos);
	   if(endPos == -1) {
		   parser->name = value.substring(startPos, value.length() - 1);
	   }
	   else {
		   parser->name = value.substring(startPos, endPos -1);
	   }
   }

   return 0;
}

int MultiPartParser::partData(multipart_parser_t* p, const char *at, size_t length)
{
	MultiPartParser* parser = (MultiPartParser *)p->data;
	if(parser == NULL) {
		return -1;
	}

	parser->params[parser->name] += String(at, length);
	return 0;
}

int MultiPartParser::partEnd(multipart_parser_t* p)
{
	MultiPartParser* parser = (MultiPartParser *)p->data;
	if(parser == NULL) {
		return -1;
	}

	parser->request->postParams[parser->name] = parser->params[parser->name];

	return 0;
}

int MultiPartParser::bodyEnd(multipart_parser_t* p)
{
	MultiPartParser* parser = (MultiPartParser *)p->data;
	if(parser == NULL) {
		return -1;
	}

	for(int i=0; i < parser->params.count(); i++) {
		String name = parser->params.keyAt(i);
		String value = parser->params.valueAt(i);
		parser->request->postParams[name] = value;
	}

	return 0;
}

void formMultipartParser(HttpRequest& request, const char *at, int length)
{
	MultiPartParser* parser = (MultiPartParser*)request.args;

	if(length == -1) {
		if(parser != NULL) {
			delete parser;
		}

		parser = new MultiPartParser(&request);
		request.args = parser;

		return;
	}

	if(length == -2) {
		if(parser != NULL) {
			delete parser;
		}
		return;
	}

	parser->execute(at, length);
}
#endif

void formUrlParser(HttpRequest& request, const char *at, int length)
{
	FormUrlParserState* state = (FormUrlParserState*)request.args;

	if(length == -1) {
		if(state != NULL) {
			delete state;
		}
		state = new FormUrlParserState;
		request.args = (void *)state;
		return;
	}

	if(length == -2) {
		int maxLength = 0;
		for(int i=0; i<request.postParams.count(); i++) {
			int kLength = request.postParams.keyAt(i).length();
			int vLength = request.postParams.valueAt(i).length();
			if(maxLength < vLength || maxLength < kLength) {
				maxLength = (kLength < vLength ? vLength : kLength);
			}
		}

		char* buffer = new char[maxLength + 1];
		for(int i=0, max = request.postParams.count(); i< max; i++) {
			String key = request.postParams.keyAt(i);
			String value = request.postParams.valueAt(i);

			uri_unescape(buffer, maxLength, key.c_str(), key.length());
			String newKey = buffer;

			if(newKey != key) {
				request.postParams.remove(key);
			}

			uri_unescape(buffer, maxLength, value.c_str(), value.length());
			request.postParams[newKey] = buffer;
		}
		delete[] buffer;

		if(state != NULL) {
			delete state;
			request.args = NULL;
		}

		return;
	}

	if(state == NULL) {
		debugf("Invalid request argument");
		return;
	}

	String data = String(at, length);

	while(data.length()) {
		int pos = data.indexOf(state->searchChar);
		if(pos == -1) {
			if(state->searchChar == '=') {
				state->postName += data;
			}
			else {
				request.postParams[state->postName] += data;
			}

			return;
		}

		String buf = data.substring(0, pos);
		if(state->searchChar == '=') {
			state->postName += buf;
			state->searchChar = '&';
		}
		else {
			request.postParams[state->postName] += buf;
			state->searchChar = '=';
			state->postName = "";
		}

		data = data.substring(pos + 1);
	}
}
