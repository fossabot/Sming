/*
 * rBootHttpUpdate.h
 *
 *  Created on: 2015/09/03.
 *      Author: Richard A Burton & Anakod
 */

#ifndef SMINGCORE_NETWORK_RBOOTHTTPUPDATE_H_
#define SMINGCORE_NETWORK_RBOOTHTTPUPDATE_H_

#include <Timer.h>

#include <rboot-api.h>
#include "WebClient.h"
#include "../OutputStream.h"

#define NO_ROM_SWITCH 0xff

class rBootHttpUpdate;

//typedef void (*otaCallback)(bool result);
typedef Delegate<void(rBootHttpUpdate& client, bool result)> OtaUpdateDelegate;

struct rBootHttpUpdateItem {
	String url;
	uint32_t targetOffset;
	int size;
};

class rBootItemOutputStream: public IOutputStream {
public:
	rBootItemOutputStream(rBootHttpUpdateItem& item) {
		this->item = item;
		rBootWriteStatus = rboot_write_init( item.targetOffset );
	}

	virtual size_t write(const uint8_t* data, size_t size) {
		if(!rboot_write_flash(&rBootWriteStatus, (uint8 *)data, size)) {
			return -1;
		}

		item.size += size;

		return size;
	}

	virtual bool close() {
		return rboot_write_end(&rBootWriteStatus);
	}

	virtual ~rBootItemOutputStream() {
		close();
	}

private:
	rBootHttpUpdateItem item;
	rboot_write_status rBootWriteStatus;
};

class rBootHttpUpdate: protected HttpClient {

public:
	rBootHttpUpdate();
	virtual ~rBootHttpUpdate();
	void addItem(int offset, String firmwareFileUrl);
	void start();
	void switchToRom(uint8 romSlot);
	void setCallback(OtaUpdateDelegate reqUpdateDelegate);
	void setDelegate(OtaUpdateDelegate reqUpdateDelegate);

	/* Sets the base request that can be used to pass
	 * - default request parameters, like request headers...
	 * - default SSL options
	 * - default SSL fingeprints
	 * - default SSL client certificates
	 *
	 * @param WebRequest *
	 */
	void setBaseRequest(WebRequest *request);

	// Allow reading items
	rBootHttpUpdateItem getItem(unsigned int index);

protected:
	void applyUpdate();
	void updateFailed();

	virtual int itemComplete(HttpConnection& client, bool success);
	virtual int updateComplete(HttpConnection& client, bool success);


protected:
	Vector<rBootHttpUpdateItem> items;
	int currentItem;
	rboot_write_status rBootWriteStatus;
	uint8 romSlot;
	OtaUpdateDelegate updateDelegate;

	WebRequest* baseRequest = NULL;
};

#endif /* SMINGCORE_NETWORK_RBOOTHTTPUPDATE_H_ */
