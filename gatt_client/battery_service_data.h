/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      battery_service_data.h
 *
 *  DESCRIPTION
 *      Header file for discovered Battery Service data and its prototypes
 *
 ******************************************************************************/

#ifndef __BATTERY_SERVICE_DATA_H__
#define __BATTERY_SERVICE_DATA_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <bt_event_types.h> /* Type definitions for Bluetooth events */

/*============================================================================*
 *  Local Header File
 *============================================================================*/

#include "battery_uuids.h"  /* Battery Service UUIDs */
#include "gatt_access.h"    /* GATT-related routines */

/*============================================================================*
 *  Public Data Types
 *============================================================================*/

/* Battery Service characteristics enumerated type */
typedef enum {
    battery_level = 0,      /* Battery Level */
    battery_type_invalid
} battery_char_t;

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Battery Service callback function table */
extern SERVICE_FUNC_POINTERS_T BatteryServiceFuncStore;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceUuid
 *
 *  DESCRIPTION
 *      Return the Service UUID and type (16- or 128-bit)
 *
 *  PARAMETERS
 *      type [out]              UUID type
 *      uuid [out]              Service UUID
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BatteryServiceUuid(GATT_UUID_T *type, uint16 *uuid);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceDataInit
 *
 *  DESCRIPTION
 *      Initialise service data during the Discovery Procedure when the service
 *      has been discovered in the Server's GATT Database.
 *
 *  PARAMETERS
 *      dev [in]                Device to initialise service data for
 *      p_event_data [in]       Primary service discovery event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BatteryServiceDataInit(uint16 dev, 
                               GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceCheckHandle
 *
 *  DESCRIPTION
 *      Check if the specified handle belongs to the service.
 *
 *  PARAMETERS
 *      dev [in]                Device to check handle for
 *      handle [in]             Handle to check
 *
 *  RETURNS
 *      TRUE if the supplied handle belongs to this service
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceCheckHandle(uint16 dev, uint16 handle);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceGetHandles
 *
 *  DESCRIPTION
 *      This function is called during the Discovery Procedure. Its behaviour
 *      depends on the value of 'type':
 *
 *      service_type:        Return the full range of characteristic handles
 *                           supported by this service
 *      characteristic_type: Return the full range of descriptor handles
 *                           supported by this characteristic
 *
 *  PARAMETERS
 *      dev [in]                Device to return handle range for
 *      start_hndl [out]        Start of handle range, or INVALID_ATT_HANDLE
 *      end_hndl [out]          End of handle range, or INVALID_ATT_HANDLE
 *      type [in]               Type of handles to return
 *
 *  RETURNS
 *      TRUE if type is service_type
 *      TRUE if type is characteristic type and there are more characteristics
 *      to be discovered.
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceGetHandles(uint16 dev,
                                     uint16 *start_hndl,
                                     uint16 *end_hndl,
                                     gatt_profile_hierarchy_t type);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceCharDiscovered
 *
 *  DESCRIPTION
 *      This function is called during the Discovery Procedure after a service
 *      characteristic has been discovered.
 *
 *  PARAMETERS
 *      dev [in]                Device on which characteristic has been
 *                              discovered
 *      p_event_data [in]       Characteristic discovery event data
 *
 *  RETURNS
 *      TRUE if the discovered characteristic is supported by this service
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceCharDiscovered(uint16 dev,
                                       GATT_CHAR_DECL_INFO_IND_T *p_event_data);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceCharDescDisc
 *
 *  DESCRIPTION
 *      This function is called during the Discovery Procedure after a
 *      characteristic descriptor has been discovered.
 *
 *  PARAMETERS
 *      dev [in]                Device on which descriptor has been discovered
 *      p_event_data [in]       Descriptor discovery event data
 *
 *  RETURNS
 *      TRUE if the discovered characteristic descriptor is supported by this
 *      service
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern void BatteryServiceCharDescDisc(uint16 dev, 
                                       GATT_CHAR_DESC_INFO_IND_T *p_event_data);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceDiscoveryComplete
 *
 *  DESCRIPTION
 *      This function called when the discovery of this service is complete.
 *      Although GATT write/read requests are supported, it is highly
 *      recommended that the full Discovery Procedure be completed before
 *      GATT read/write procedures are initiated.
 *
 *  PARAMETERS
 *      dev [in]                Device on which service discovery has completed
 *      connect_handle [in]     Handle of connection with the device
 *
 *  RETURNS
 *      TRUE if a GATT read/write request is initated by this function
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceDiscoveryComplete(uint16 dev,
                                            uint16 connect_handle);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceHandlerNotifInd
 *
 *  DESCRIPTION
 *      Handle GATT_IND_CHAR_VAL_IND and GATT_NOT_CHAR_VAL_IND events for this
 *      service.
 *
 *  PARAMETERS
 *      dev [in]                Device on which notification/indication has
 *                              arisen
 *      handle [in]             Characteristic affected
 *      size [in]               Characteristic value size
 *      value [in]              New characteristic value
 *
 *  RETURNS
 *      TRUE on success, FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceHandlerNotifInd(uint16 dev,
                                          uint16 handle,
                                          uint16 size,
                                          uint8  *value);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceConfigNotif
 *
 *  DESCRIPTION
 *      Update the value of the specified descriptor for the specified
 *      characteristic of this service according to the value of 'Enable'.
 *
 *  PARAMETERS
 *      dev [in]                Device on which characteristic descriptor
 *                              should be updated
 *      Type [in]               Characteristic the descriptor belongs to
 *      SubType [in]            Characteristic descriptor to update
 *      Enable [in]             New characteristic descriptor value
 *
 *  RETURNS
 *      TRUE on success, FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceConfigNotif(uint16 dev,
                                      uint16 Type,
                                      uint8 SubType,
                                      bool Enable);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceReadRequest
 *
 *  DESCRIPTION
 *      Initiate a GATT read request for the specified characteristic value.
 *
 *  PARAMETERS
 *      dev [in]                Device on which to read characteristic value
 *      type [in]               Characteristic to read
 *
 *  RETURNS
 *      TRUE on success, FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceReadRequest(uint16 dev, uint16 type);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceReadConfirm
 *
 *  DESCRIPTION
 *      Called when a read request is successful.
 *
 *  PARAMETERS
 *      dev [in]                Device on characteristic value was read
 *      size [in]               Size of data returned, in octets
 *      value [in]              Characteristic value
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BatteryServiceReadConfirm(uint16 dev, uint16 size, uint8 *value);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceWriteConfirm
 *
 *  DESCRIPTION
 *      Called when a write request is successful.
 *
 *  PARAMETERS
 *      dev [in]                Device on characteristic value was read
 *      connect_handle [in]     Device connection handle
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BatteryServiceWriteConfirm(uint16 dev, 
                                       uint16 connect_handle);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceConfigure
 *
 *  DESCRIPTION
 *      Configure the Server's GATT database for this service.
 *
 *  PARAMETERS
 *      dev [in]                Device to configure
 *
 *  RETURNS
 *      TRUE if notification is initiated for the current characteristic
 *      FALSE when configuration is complete
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceConfigure(uint16 dev);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceFound
 *
 *  DESCRIPTION
 *      Check if this service is supported on the specified device.
 *
 *  PARAMETERS
 *      dev [in]                Device to check
 *
 *  RETURNS
 *      TRUE if this service is supported on the specified device.
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool BatteryServiceFound(uint16 dev);

/*---------------------------------------------------------------------------
 *  NAME
 *      BatteryServiceResetData
 *
 *  DESCRIPTION
 *      Reset the service data for the specified device.
 *
 *  PARAMETERS
 *      dev [in]                Device to reset data for
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void BatteryServiceResetData(uint16 dev);

#endif /* __BATTERY_SERVICE_DATA_H__ */

