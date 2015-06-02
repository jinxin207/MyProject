/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      dev_info_uuids.h
 *
 *  DESCRIPTION
 *      UUID MACROs for Device Information Service
 *
 *****************************************************************************/

#ifndef __DEV_INFO_UUIDS_H__
#define __DEV_INFO_UUIDS_H__

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Brackets should not be used around the value of these macros. gattdbgen which 
 * creates .c and .h files from .db files does not support brackets and will
 * raise syntax errors. 
 */

/* For UUID values, refer http://developer.bluetooth.org/gatt/services/
 * Pages/ServiceViewer.aspx?u=org.bluetooth.service.device_information.xml
 */

#define UUID_DEVICE_INFO_SERVICE                         0x180A

#define UUID_DEVICE_INFO_SYSTEM_ID                       0x2A23

#define UUID_DEVICE_INFO_MODEL_NUMBER                    0x2A24

#define UUID_DEVICE_INFO_SERIAL_NUMBER                   0x2A25

#define UUID_DEVICE_INFO_HARDWARE_REVISION               0x2A27

#define UUID_DEVICE_INFO_FIRMWARE_REVISION               0x2A26

#define UUID_DEVICE_INFO_SOFTWARE_REVISION               0x2A28

#define UUID_DEVICE_INFO_MANUFACTURER_NAME               0x2A29

#define UUID_DEVICE_INFO_PNP_ID                          0x2A50

#define UUID_DEVICE_IEEE_REG_CERTI_DATA_LIST             0x2A2A

/* Vendor Id Source */
#define VENDOR_ID_SRC_BT                                 0x01
#define VENDOR_ID_SRC_USB                                0x02

/* Vendor Id - CSR*/
#define VENDOR_ID                                        0x000A
#define PRODUCT_ID                                       0x014C
#define PRODUCT_VER                                      0x0100

#if defined(CSR101x_A05)
    #define HARDWARE_REVISION "CSR101x A05"
#else
    #define HARDWARE_REVISION "Unknown"
#endif

#endif /* __DEV_INFO_UUIDS_H__ */
