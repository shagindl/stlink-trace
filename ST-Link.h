#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "stlink-trace.h"

#ifdef __cplusplus

class DLL_EXPORT STLink_SWO_Sniffer {
private:
	bool state = false;

	libusb_hndl_t libusb_hndl;
	libusb_device* stlinkdev = nullptr;

	ssize_t listSize = 0;
	struct libusb_transfer* responseTransfer = 0;
	struct libusb_transfer* requestTransfer = 0;

	stlink_t stlinkl{ libusb_hndl };

	Logger& logger;
public:
	STLink_SWO_Sniffer(Logger* pLogger) : logger(*pLogger) {
		stlinkl.Init(pLogger);
	}
	~STLink_SWO_Sniffer() {
		Clean();
	}

	bool Open();
	int Send(char* buff, int szBuff) { return 0; /* @TODO: */ }
	int Recv(char* buff, int szBuff);
	void Clean();
};

#endif