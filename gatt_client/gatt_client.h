/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gatt_client.h
 *
 *  DESCRIPTION
 *      Header file for a simple GATT client application.
 *
 ******************************************************************************/

#ifndef __GATT_CLIENT_H__
#define __GATT_CLIENT_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>          /* Commonly used type definitions */
#include <bluetooth.h>      /* Bluetooth specific type definitions */
#include <timer.h>          /* Chip timer functions */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "gatt_access.h"    /* GATT-related routines */

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Maximum number of words in GAP central Identity Resolution Key (IRK) */
#define MAX_WORDS_IRK                       (8)

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      ReportPanic
 *
 *  DESCRIPTION
 *      This function calls firmware panic routine and gives a single point 
 *      of debugging any application level panics.
 *
 *  PARAMETERS
 *      panic_code [in]         Code to supply to firmware Panic function.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void ReportPanic(app_panic_code panic_code);

/*----------------------------------------------------------------------------*
 *  NAME
 *      DeviceFound
 *
 *  DESCRIPTION
 *      This function is called when a new device is discovered during the
 *      scan. It stores the information in the application data structure.
 *
 *  PARAMETERS
 *      disc_device [in]        Information for the discovered device
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void DeviceFound(DISCOVERED_DEVICE_T *disc_device);

/*----------------------------------------------------------------------------*
 *  NAME
 *      NotifyServiceFound
 *
 *  DESCRIPTION
 *      This function is used for notifying the application about the
 *      discovered service
 *
 *  PARAMETERS
 *      pService [in]           Discovered service callback function table
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void NotifyServiceFound(SERVICE_FUNC_POINTERS_T *pService);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GetConnServices
 *
 *  DESCRIPTION
 *      This function returns a pointer to all the connected services
 *
 *  PARAMETERS
 *      dev [out]               Connected device number. Set to NULL if this
 *                              information is not required.
 *      totalServices [out]     Total number of connected services. Set to NULL
 *                              if this information is not required.
 *
 *  RETURNS
 *      Pointer to array of connected services' callback function tables
 *----------------------------------------------------------------------------*/
extern SERVICE_FUNC_POINTERS_T **GetConnServices(uint16 *dev,
                                                 uint16 *totalServices);

/*----------------------------------------------------------------------------*
 *  NAME
 *      SetState
 *
 *  DESCRIPTION
 *      This function is used to set the state of the application.
 *
 *  PARAMETERS
 *      dev [in]                Device number to change state for
 *      new_state [in]          New state to move to
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void SetState(uint16 dev, app_state new_state);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GetState
 *
 *  DESCRIPTION
 *      This function returns the current state of the application for the 
 *      specified device. 
 *
 *  PARAMETERS
 *      dev [in]                Device number for which to return state
 *
 *  RETURNS
 *      Current state of the specified device
 *----------------------------------------------------------------------------*/
extern app_state GetState(uint16 dev);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GetConnDevice
 *
 *  DESCRIPTION
 *      This function returns the connected device for which GATT procedure 
 *      is being performed. 
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Device number of currently connected device
 *----------------------------------------------------------------------------*/
extern uint16 GetConnDevice(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      DisconnectDevice
 *
 *  DESCRIPTION
 *      Disconnect the specified device.
 *
 *  PARAMETERS
 *      dev [in]                Number of device to disconnect.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void DisconnectDevice(uint16 dev);

/*----------------------------------------------------------------------------*
 *  NAME
 *      DeviceConfigured
 *
 *  DESCRIPTION
 *      Notifies the application that the specified device has been configured
 *      for all the connected services.
 *
 *  PARAMETERS
 *      dev [in]                Number of configured device
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void DeviceConfigured(uint16 dev);

#ifdef PAIRING_SUPPORT
/*----------------------------------------------------------------------------*
 *  NAME
 *      StartBonding
 *
 *  DESCRIPTION
 *      Start a timer that on expiry triggers the Pairing Procedure, if the
 *      peer device has a Resolvable Random Address and has not already
 *      initiated pairing by that time.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void StartBonding(void);
#endif /* PAIRING_SUPPORT */

/*----------------------------------------------------------------------------*
 *  NAME
 *      NextReadWriteProcedure
 *
 *  DESCRIPTION
 *      This function initiates any read/write procedures. If argument 'next' is
 *      TRUE, it will initiate the next argument, otherwise it will initiate the 
 *      procedure for the current characteristic.
 *
 *  PARAMETERS
 *      next [in]               Whether to initiate the next argument
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void NextReadWriteProcedure(bool next);

#endif /* __GATT_CLIENT_H__ */
