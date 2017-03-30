/*
 * rBootHttpUpdate.cpp
 *
 *  Created on: 2015/09/03.
 *      Author: Richard A Burton & Anakod
 */

#include "rBootHttpUpdate.h"
#include "../Platform/System.h"
#include "URL.h"
#include "../Platform/WDT.h"

rBootHttpUpdate::rBootHttpUpdate() {
	currentItem = 0;
	romSlot = NO_ROM_SWITCH;
	updateDelegate = nullptr;
}

rBootHttpUpdate::~rBootHttpUpdate() {
}

void rBootHttpUpdate::addItem(int offset, String firmwareFileUrl) {
	rBootHttpUpdateItem add;
	add.targetOffset = offset;
	add.url = firmwareFileUrl;
	add.size = 0;
	items.add(add);
}

void rBootHttpUpdate::setBaseRequest(WebRequest *request) {
	baseRequest = request;
}

void rBootHttpUpdate::start() {
	for(int i=0; i< items.count(); i++) {
		rBootHttpUpdateItem &it = items[i];
		debugf("Download file:\r\n    (%d) %s -> %X", currentItem, it.url.c_str(), it.targetOffset);

		WebRequest *request;
		if(baseRequest != NULL) {
			request = baseRequest->clone();
			request->setURL(URL(it.url));
		}
		else {
			request = new WebRequest(URL(it.url));
		}

		request->setMethod(HTTP_GET);

		rBootItemOutputStream *responseStream = new rBootItemOutputStream(&it);
		request->setResponseStream(responseStream);

		if(i == items.count() - 1) {
			request->onRequestComplete(RequestCompletedDelegate(&rBootHttpUpdate::updateComplete, this));
		}
		else {
			request->onRequestComplete(RequestCompletedDelegate(&rBootHttpUpdate::itemComplete, this));
		}

		if(!send(request)) {
			debugf("ERROR: Rejected sending new request.");
			break;
		}
	}
}

int rBootHttpUpdate::itemComplete(HttpConnection& client, bool success) {
	if(!success) {
		updateFailed();
		return -1;
	}

	return 0;
}

int rBootHttpUpdate::updateComplete(HttpConnection& client, bool success) {
	debugf("\r\nFirmware download finished!");
	for (int i = 0; i < items.count(); i++) {
		debugf(" - item: %d, addr: %X, len: %d bytes", i, items[i].targetOffset, items[i].size);
	}

	if(!success) {
		updateFailed();
		return -1;
	}

	if(updateDelegate) {
		updateDelegate(*this, true);
	}

	applyUpdate();

	return 0;
}

void rBootHttpUpdate::switchToRom(uint8 romSlot) {
	this->romSlot = romSlot;
}

void rBootHttpUpdate::setCallback(OtaUpdateDelegate reqUpdateDelegate) {
	setDelegate(reqUpdateDelegate);
}

void rBootHttpUpdate::setDelegate(OtaUpdateDelegate reqUpdateDelegate) {
	this->updateDelegate = reqUpdateDelegate;
}

void rBootHttpUpdate::updateFailed() {
	debugf("\r\nFirmware download failed..");
	if (updateDelegate) {
		updateDelegate(*this, false);
	}
	items.clear();
}

void rBootHttpUpdate::applyUpdate() {
	items.clear();
	if (romSlot == NO_ROM_SWITCH) {
		debugf("Firmware updated.");
		return;
	}

	// set to boot new rom and then reboot
	debugf("Firmware updated, rebooting to rom %d...\r\n", romSlot);
	rboot_set_current_rom(romSlot);
	System.restart();
}

rBootHttpUpdateItem rBootHttpUpdate::getItem(unsigned int index) {
	return items.elementAt(index);
}
