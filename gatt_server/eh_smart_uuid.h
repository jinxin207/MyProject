/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      battery_uuids.h
 *
 *  DESCRIPTION
 *      UUID MACROs for the Battery Service
 *
 *****************************************************************************/

#ifndef __BATTERY_UUIDS_H__
#define __BATTERY_UUIDS_H__

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Brackets should not be used around the values of these macros. This file is
 * imported by the GATT Database Generator (gattdbgen) which does not understand 
 * brackets and will raise syntax errors.
 */

/* For UUID values, refer http://developer.bluetooth.org/gatt/services/
 * Pages/ServiceViewer.aspx?u=org.bluetooth.service.battery_service.xml
 */

#define UUID_SMART_HOME_SERVICE                           0xF015ED01000011112222333344445555

#define UUID_SMART_HOME_SENSOR                             0xF015FF01000011112222333344445555
#define UUID_SMART_HOME_CONTROL                             0xF015FF02000011112222333344445555

#endif /* __BATTERY_UUIDS_H__ */
