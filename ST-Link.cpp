#include "ST-Link.h"

#pragma comment( lib, "libusb-1.0.lib" )


bool STLink_SWO_Sniffer::Open() {
    int ret, pos, opt = 0;
    auto& ctx = libusb_hndl.ctx;
    auto& stlinkhandle = libusb_hndl.stlinkhandle;
    auto& deviceList = libusb_hndl.deviceList;
    auto& requestTransfer = libusb_hndl.requestTransfer;
    auto& responseTransfer = libusb_hndl.responseTransfer;

    // initialise the USB session context
    ret = libusb_init(&ctx);
    if (ret != 0) {
        printf("Error initialising libusb: 0x%x\n", ret);
        stlinkl.Cleanup();
        return false;
    }

    // turn debug messages on - full logging
    libusb_set_debug(ctx, DEBUG_LEVEL);

    // enumerate the USB devices
    listSize = libusb_get_device_list(ctx, &deviceList);
    for (pos = 0; pos < listSize; pos++) {
        if (stlinkl.IsStlink(deviceList[pos])) {
            stlinkdev = deviceList[pos];
            break;
        }
    }

    if (stlinkdev == NULL) {
        printf("Unable to locate an ST-Link V2 device.\n");
        return false;
    }

    // open ST-Link V2 adapter
    ret = libusb_open(stlinkdev, &(stlinkhandle));
    if (ret != 0) {
        printf("Unable to open ST-Link V2 device.\n");
        stlinkl.Cleanup();
        return false;
    }

    // detach from kernel if required
    if (libusb_kernel_driver_active(stlinkhandle, 0)) {
        printf("Detaching the device from the kernel\n");
        libusb_detach_kernel_driver(stlinkhandle, 0);
    }

    int config = 0;
    if (libusb_get_configuration(stlinkhandle, &config)) {
        printf("Unable to get configuration\n");
    }

    if (config != 1) {
        printf("setting new configuration (%d -> 1)\n", config);
        if (libusb_set_configuration(stlinkhandle, 1)) {
            printf("Unable to set configuration\n");
        }
    }

    ret = libusb_claim_interface(stlinkhandle, 0);
    if (ret != 0) {
        printf("Unable to claim interface.\n");
        stlinkl.Cleanup();
        return false;
    }

    requestTransfer = libusb_alloc_transfer(0);
    if (requestTransfer == NULL) {
        printf("Allocation of request transfer failed.\n");
        stlinkl.Cleanup();
        return false;
    }

    responseTransfer = libusb_alloc_transfer(0);
    if (responseTransfer == NULL) {
        printf("Allocation of response transfer failed.\n");
        stlinkl.Cleanup();
        return false;
    }

    responseTransfer->flags &= ~LIBUSB_TRANSFER_SHORT_NOT_OK;

    //============================
    // identify the microcontroller, set up the debugging and step through instructions to get the trace data
    //============================

    stlinkl.GetCurrentMode();
    stlinkl._GetVersion();

    //libusb_clear_halt(stlinkhandle, 0x81);

    stlinkl.ExitDFUMode();

    if (stlinkl.GetCurrentMode() != MODE_DBG) {
        stlinkl.EnterSWD();
    }

    stlinkl.GetTargetVoltage();
    stlinkl.EnterDebugState();
    stlinkl.GetCoreId();

    // @TODO: ? stlinkl.ResetCore();
    stlinkl.ForceDebug();

    stlinkl.EnableTrace();
    stlinkl.RunCore();

    return true;
}
int STLink_SWO_Sniffer::Recv(char* buff, int szBuff){
    int byteCount = 0, bytesRead = 0, cnt_tries = 5, cnt_tries_update = 2;
    int indx_buff = 0;

    //printf("Bgn Recv.  ");
    while (cnt_tries > 0) {
        stlinkl.usleep(1000), cnt_tries--;

        //byteCount = 256;
        byteCount = stlinkl.FetchTraceByteCount();
        //printf("cnt_tries{ %d }; byteCount{ %d }", cnt_tries, byteCount);

        if (byteCount == 0) continue;
        else {
            cnt_tries += cnt_tries_update;
            if (cnt_tries_update > 0) cnt_tries_update--;
        }

        if (byteCount > szBuff - indx_buff) {
            printf("Detected byteCount{%d} > szBuff{%d}./n", byteCount, szBuff - indx_buff);
            byteCount = szBuff - indx_buff;
        }

        if (byteCount >= 2048) {
            int toread = 0;
            printf("**** more than 2048 bytes in trace data!! : 0x%4x ****\n", byteCount);

            if ((byteCount & 0xF800) == 0xF800) {
                //byteCount -= 0xF800;
                printf("Detected overrun packet?/n");
                //ReadTraceData(1, byteCount);
                //ForceDebug();
                //RunCore();	// run it - stalled?
                //continue;
            }
            while (byteCount > 0) {
                toread = byteCount > min(2048, szBuff - indx_buff) ? min(2048, szBuff - indx_buff) : byteCount;
                bytesRead += stlinkl.ReadTraceData(0, toread, &buff[indx_buff]);
                byteCount -= toread;
                indx_buff += toread;

                // check the register values
                //unsigned int value =
                stlinkl.ReadDHCSRValue();
                //printf("DHCSR = 0x%04x\n", value);
            }

            stlinkl.ForceDebug();
            stlinkl.RunCore();	// run it - stalled?

            continue;
        }

        bytesRead = stlinkl.ReadTraceData(0, byteCount, &buff[indx_buff]);
        indx_buff += bytesRead;
        /*printf("bytesRead = %d, byteCount = %d, indx_buff = %d. cnt_tries = %d.\r\n",
                bytesRead, byteCount, indx_buff, cnt_tries);*/
    }
    //printf("End Recv.\r\n");
    
    return indx_buff;
}
void STLink_SWO_Sniffer::Clean() {
    auto ret = libusb_release_interface(libusb_hndl.stlinkhandle, 0);
    if (ret != 0) {
        printf("Unable to release interface.\n");
        stlinkl.Cleanup();
    } else {
        // finished - clean everything
        stlinkl.Cleanup();
    }
}
