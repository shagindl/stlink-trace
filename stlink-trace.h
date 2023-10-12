/*
 * stlink-trace.h
 *
 *  Created on: 21/02/2013
 *      Author: Chris Storah
 *
 * Parts of the ST-Link code comes from the stlink project:
 * https://github.com/texane/stlink
 *
 */

#ifndef STLINK_TRACE_H_
#define STLINK_TRACE_H_

#ifdef DLL
    #define DLL_EXPORT   __declspec( dllexport )
#else
    #define DLL_EXPORT
#endif /* DLL */

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <stdio.h>
#include "fwm-common\modules\Logger\Logger.h"
#include "libusb-1.0/libusb.h"

// Bus 002 Device 052: ID 0483:3748 SGS Thomson Microelectronics ST-LINK/V2

#define STLINKV2_VENDOR_ID    0x0483
#define STLINKV2_PRODUCT_ID   0x3748

#define DEBUG_LEVEL           3

#define DEBUG_COMMAND         0xF2

#define READ32                0x07
#define WRITE32               0x08

#define WRITE_DATA            0x35
#define READ_DATA             0x36

/*
 * USB modes:
 * DFU = Device Firmware Update
 * MSD = Mass Storage Device
 * DBG = Debug
 */
#define MODE_DFU              0x0000
#define MODE_MSD              0x0001
#define MODE_DBG              0x0002

#define STLINK_DEBUG_COMMAND  0xF2
#define STLINK_DFU_COMMAND    0xF3
#define STLINK_DFU_EXIT       0x07
//?? #define STLINK_DFU_ENTER      0x08

#define STLINK_DEBUG_FORCEDEBUG  0x02
#define STLINK_DEBUG_RESETSYS    0x03

struct libusb_hndl_t {
    libusb_context* ctx = nullptr;
    libusb_device_handle* stlinkhandle = nullptr;
    libusb_device** deviceList = nullptr;
};

class DLL_EXPORT stlink_t {
    libusb_context* &ctx;
    libusb_device_handle* &stlinkhandle;
    libusb_device** &deviceList;

    int debugEnabled = 0;

    FILE* resultsFile = NULL;
    FILE* fullResultsFile = NULL;
    Logger *logger = nullptr;
public:
    stlink_t(libusb_hndl_t& hndl) : ctx(hndl.ctx), stlinkhandle(hndl.stlinkhandle), deviceList(hndl.deviceList){

    }
    void Init(Logger* pLogger) {
        logger = pLogger;
    }
    void Cleanup();
    int IsStlink(libusb_device* dev);
    void _GetVersion();
    int GetCurrentMode();
    void ExitDFUMode();
    void EnterSWD();
    void RunCore();
    void GetTargetVoltage();
    void EnterDebugState();
    void GetCoreId();
    void ResetCore();
    void ForceDebug();
    void EnableTrace(bool fNeedReset = false);
    void usleep(unsigned int usec);
    int FetchTraceByteCount();
    int ReadTraceData(int toscreen, int byteCount, char* out = nullptr);
    uint32_t ReadDHCSRValue();
private:
    void Write32Bit(uint32_t address, uint32_t value);
    uint32_t Read32Bit(uint32_t address);
    void UnknownCommand();
    void LocalReset();
    int SendAndReceive(unsigned char* txBuffer, size_t txSize, unsigned char* rxBuffer, size_t rxSize);
    void HaltRunningSystem();
    void StepCore();
    ssize_t TransferData(int terminate,
        unsigned char* transmitBuffer, size_t transmitLength,
        unsigned char* receiveBuffer, size_t receiveLength);
};

#endif /* STLINK_TRACE_H_ */
