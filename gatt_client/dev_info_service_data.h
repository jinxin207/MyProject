/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      dev_info_service_data.h
 *
 *  DESCRIPTION
 *      Header file for discovered Device Info Service data and its prototypes
 *
 ******************************************************************************/

#ifndef __DEV_INFO_SERVICE_DATA_H__
#define __DEV_INFO_SERVICE_DATA_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <bt_event_types.h> /* Type definitions for Bluetooth events */

/*============================================================================*
 *  Local Header File
 *============================================================================*/

#include "dev_info_uuids.h" /* Device Information Service UUIDs */
#include "gatt_access.h"    /* GATT-related routines */

/*============================================================================*
 *  Public Data Types
 *============================================================================*/

/* Device Information Service characteristics enumerated type */
typedef enum {
    dev_info_manufacture_name = 0,      /* Manufacturer Name String */
    dev_info_model_num,                 /* Model Number String */
    dev_info_serial_num,                /* Serial Number String */
    dev_info_hw_rev,                    /* Hardware Revision String */
    dev_info_fw_rev,                    /* Firmware Revision String */
    dev_info_sw_rev,                    /* Software Revision String */
    dev_info_sys_id,                    /* System ID */
    dev_info_certi_list,                /* IEEE 11073-20601 Regulatory 
                                         * Certification Data List 
                                         */
    dev_info_pnp_id,                    /* PnP ID */
    dev_info_type_invalid
} dev_info_char_t;

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Device Information Service callback function table */
extern SERVICE_FUNC_POINTERS_T DeviceInfoServiceFuncStore;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceUuid
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
extern void DeviceInfoServiceUuid(GATT_UUID_T *type, uint16 *uuid);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceDataInit
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
extern void DeviceInfoServiceDataInit(uint16 dev, 
                               GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceCheckHandle
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
extern bool DeviceInfoServiceCheckHandle(uint16 dev,
                                         uint16 handle);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceGetHandles
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
extern bool DeviceInfoServiceGetHandles(uint16 dev,
                                        uint16 *start_hndl,
                                        uint16 *end_hndl,
                                        gatt_profile_hierarchy_t type);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceCharDiscovered
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
extern bool DeviceInfoServiceCharDiscovered(uint16 dev,
                                       GATT_CHAR_DECL_INFO_IND_T *p_event_data);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceCharDescDisc
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
extern void DeviceInfoServiceCharDescDisc(uint16 dev, 
                                       GATT_CHAR_DESC_INFO_IND_T *p_event_data);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceDiscoveryComplete
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
extern bool DeviceInfoServiceDiscoveryComplete(uint16 dev,
                                               uint16 connect_handle);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceReadRequest
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
extern bool DeviceInfoServiceReadRequest(uint16 dev, uint16 type);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceReadConfirm
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
extern void DeviceInfoServiceReadConfirm(uint16 dev, 
                                         uint16 size,
                                         uint8 *value);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceFound
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
extern bool DeviceInfoServiceFound(uint16 dev);

/*---------------------------------------------------------------------------
 *  NAME
 *      DeviceInfoServiceResetData
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
extern void DeviceInfoServiceResetData(uint16 dev);

#endif /* __DEV_INFO_SERVICE_DATA_H__ */
