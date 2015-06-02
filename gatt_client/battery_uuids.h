/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      battery_uuids.h
 *
 *  DESCRIPTION
 *      UUID MACROs for Battery Service
 *
 *****************************************************************************/

#ifndef __BATTERY_UUIDS_H__
#define __BATTERY_UUIDS_H__

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Brackets should not be used around the value of these macros. gattdbgen which 
 * creates .c and .h files from .db files does not support brackets and will
 * raise syntax errors. 
 */

/* For UUID values, refer http://developer.bluetooth.org/gatt/services/
 * Pages/ServiceViewer.aspx?u=org.bluetooth.service.battery_service.xml
 */

#define UUID_BATTERY_SERVICE                           0x180f

#define UUID_BATTERY_LEVEL                             0x2a19

#endif /* __BATTERY_UUIDS_H__ */
