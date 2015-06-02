/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      dev_info_service_data.c
 *
 *  DESCRIPTION
 *      This file keeps information related to Discovered Device Info service  
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *===========================================================================*/

#include <gatt.h>           /* GATT application interface */
#include <mem.h>            /* Memory library */
#include <gatt_uuid.h>      /* Common Bluetooth UUIDs and macros */

/*============================================================================*
 *  Local Header File
 *============================================================================*/

#include "dev_info_service_data.h" /* Interface to this file */
#include "gatt_access.h"    /* GATT-related routines */
#include "debug_interface.h"/* Application debug routines */

/*============================================================================*
 *  Private Definitions
 *===========================================================================*/

/* Number of characteristics present in this service, range [1, 15] */
#define MAXIMUM_NUMBER_OF_CHARACTERISTIC              (9)

/*============================================================================*
 *  Private Data Types
 *===========================================================================*/

/* Device Information Service structure used in discovery procedure */
typedef struct _DEV_INFO_SERVICE_DATA_T
{
    /* Service attribute range */
    uint16 service_start_handle;
    uint16 service_end_handle;

    /* Connection handle */
    uint16 connect_handle;

    /* Characteristics */
    CHARACTERISTIC_T chars[MAXIMUM_NUMBER_OF_CHARACTERISTIC];

    /* Total number of supported characteristics found for this service in the
     * Server's GATT Database (number of entries in chars array). The optimal
     * value is MAXIMUM_NUMBER_OF_CHARACTERISTIC
     */
    uint16 total_char:4;
    
    /* Index into chars array of the current characteristic */
    uint16 curr_char:4;

    /* Index into chars array of characteristic currently being configured
     * (unused in this service)
     */
    uint16 curr_config_char:4;

    /* Flag set to 1 if configuration is ongoing (unused in this service) */
    uint16 config_ongoing:1;

    /* Flag set to 1 if a write request initiated during configuration has
     * been confirmed (unused in this service)
     */
    uint16 write_cfm:1;

    /* Flag set to 1 if a read request initiated during configuration has
     * been confirmed (unused in this service)
     */
    uint16 read_cfm:1;

    /* Array of characteristic 'types' corresponding to the chars array. Only
     * used in read/write/notify procedures.
     */
    dev_info_char_t type[MAXIMUM_NUMBER_OF_CHARACTERISTIC];   
} DEV_INFO_SERVICE_DATA_T;

/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Device Information Service data.
 * A record is kept for each connected device supporting the service.
 */
static DEV_INFO_SERVICE_DATA_T g_dis_data[MAX_CONNECTED_DEVICES];

/*============================================================================*
 *  Public Data
 *============================================================================*/

/* Callback function table */
SERVICE_FUNC_POINTERS_T DeviceInfoServiceFuncStore = {
    .serviceUuid            = &DeviceInfoServiceUuid,
    .isMandatory            = NULL,
    .serviceInit            = &DeviceInfoServiceDataInit,
    .checkHandle            = &DeviceInfoServiceCheckHandle,
    .getHandles             = &DeviceInfoServiceGetHandles,
    .charDiscovered         = &DeviceInfoServiceCharDiscovered,
    .descDiscovered         = &DeviceInfoServiceCharDescDisc,
    .discoveryComplete      = &DeviceInfoServiceDiscoveryComplete,
    .serviceIndNotifHandler = NULL,
    .configureServiceNotif  = NULL,
    .writeRequest           = NULL,
    .writeConfirm           = NULL,
    .readRequest            = &DeviceInfoServiceReadRequest,
    .readConfirm            = &DeviceInfoServiceReadConfirm,
    .configureService       = NULL,
    .isServiceFound         = &DeviceInfoServiceFound,
    .resetServiceData       = &DeviceInfoServiceResetData
};

/*============================================================================*
 *  Private Function Prototypes
 *===========================================================================*/

/* Initialises the Device Information Service data */
static void deviceInfoDataInit(uint16 dev); 

/* Check if the characteristic supports the specified ATT permissions */
static bool deviceInfoCheckATTPermission(uint16           dev,
                                         dev_info_char_t  type, 
                                         uint16           permission,
                                         uint16          *count);

/*============================================================================*
 *  Private Function Implementations
 *===========================================================================*/

/*---------------------------------------------------------------------------
 *  NAME
 *      deviceInfoDataInit
 *
 *  DESCRIPTION
 *      Initialise the Device Information Service data.
 *
 *  PARAMETERS
 *      dev [in]                Device to initialise service data for
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void deviceInfoDataInit(uint16 dev)
{
    uint16 char_num;            /* Loop counter */
    /* Service data for the specified device */
    DEV_INFO_SERVICE_DATA_T *data = &g_dis_data[dev];

    /* Reset the data */
    data->connect_handle       = GATT_INVALID_UCID;
    data->service_start_handle = INVALID_ATT_HANDLE;
    data->service_end_handle   = INVALID_ATT_HANDLE;
    data->total_char           = 0; 
    data->curr_char            = 0;

    /* Reset characteristics array */
    MemSet(data->chars, 0x0, sizeof(data->chars));

    /* Initialise characteristic array values */
    for(char_num = 0; char_num < MAXIMUM_NUMBER_OF_CHARACTERISTIC; char_num ++)
    {
        data->type[char_num]                        = dev_info_type_invalid; 
        data->chars[char_num].valHandle             = INVALID_ATT_HANDLE;
        data->chars[char_num].descriptors[0].handle = INVALID_ATT_HANDLE;
        data->chars[char_num].descriptors[1].handle = INVALID_ATT_HANDLE;
    }
}

/*---------------------------------------------------------------------------
 *  NAME
 *      deviceInfoCheckATTPermission
 *
 *  DESCRIPTION
 *      Check whether a characteristic supports ATT permissions.
 *
 *  PARAMETERS
 *      dev [in]                Device to check
 *      type [in]               Characteristic to check
 *      permission [in]         ATT permission(s) to check
 *      count [out]             Index into service data of characteristic
 *
 *  RETURNS
 *      TRUE if the characteristic supports the specified ATT permissions on
 *      this device
 *      FALSE otherwise, or on error
 *----------------------------------------------------------------------------*/
static bool deviceInfoCheckATTPermission(uint16           dev,
                                         dev_info_char_t  type, 
                                         uint16           permission,
                                         uint16          *count)
{
    /* Initialise return value */
    *count = 0;

    /* Check the requested characteristic is supported */
    if(type >= dev_info_type_invalid)
    {
        return FALSE;
    }        

    /* Search for the specified characteristic in the chars array */
    while(g_dis_data[dev].type[*count] != type)
    {
        (*count)++;

        if(*count == MAXIMUM_NUMBER_OF_CHARACTERISTIC)
        {
            /* Characteristic not found */
            return FALSE;
        }
    }

    /* Check that the characteristic supports all the permissions requested */
    if((g_dis_data[dev].chars[*count].properties & permission) != permission) 
    {
        return FALSE;
    }

    return TRUE;
}

/*============================================================================*
 *  Public Function Implementations
 *===========================================================================*/

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
void DeviceInfoServiceUuid(GATT_UUID_T *type, uint16 *uuid)
{
    *type = GATT_UUID16;
    uuid[0] = UUID_DEVICE_INFO_SERVICE;
}

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
void DeviceInfoServiceDataInit(uint16 dev, 
                               GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data)
{
    deviceInfoDataInit(dev);
    
    g_dis_data[dev].service_start_handle = p_event_data->strt_handle;
    g_dis_data[dev].service_end_handle   = p_event_data->end_handle;
}

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
bool DeviceInfoServiceCheckHandle(uint16 dev, uint16 handle)
{
    return (((handle >= g_dis_data[dev].service_start_handle) &&
                    (handle <= g_dis_data[dev].service_end_handle))
                    ? TRUE : FALSE);
}

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
bool DeviceInfoServiceGetHandles(uint16 dev,
                                 uint16 *start_hndl,
                                 uint16 *end_hndl,
                                 gatt_profile_hierarchy_t type)
{
    switch(type)
    {
        case service_type:
        {
            *start_hndl = g_dis_data[dev].service_start_handle;
            *end_hndl   = g_dis_data[dev].service_end_handle;
        }
        break;

        case characteristic_type:
        {
            const uint16 curr_char = g_dis_data[dev].curr_char++;
            
            /* Start handle will be the 'value handle' */
            *start_hndl = g_dis_data[dev].chars[curr_char].valHandle;

            if(curr_char + 1 == g_dis_data[dev].total_char)
            {
                /* If this is the last service characteristic the end handle is
                 * the service end handle.
                 */
                *end_hndl   = g_dis_data[dev].service_end_handle;
            }
            else if(curr_char + 1 > g_dis_data[dev].total_char)
            {
                /* If there are no more characteristics populate start and end
                 * handles with INVALID_ATT_HANDLE and return FALSE 
                 */
                *start_hndl = INVALID_ATT_HANDLE;
                *end_hndl   = INVALID_ATT_HANDLE;

                return FALSE;
            }
            else
            {
                /* Otherwise the end handle is the start handle of the next
                 * characteristic less 2.
                 */
                *end_hndl = g_dis_data[dev].chars[curr_char + 1].valHandle - 2;
            }
        }
        break;

        default:
        {
            *start_hndl = INVALID_ATT_HANDLE;
            *end_hndl   = INVALID_ATT_HANDLE;
            
            /* Unsupported type passed from the application */
            return FALSE;
        }
    }

    /* More characteristics are available - return TRUE */
    return TRUE;
}

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
bool DeviceInfoServiceCharDiscovered(uint16 dev, 
                                     GATT_CHAR_DECL_INFO_IND_T *p_event_data)
{
    /* Handle of discovered characteristic */
    const uint16 handle = p_event_data->val_handle;

    /* Total number of supported characteristics discovered on this Server */
    const uint16 total_char = g_dis_data[dev].total_char;

    /* Check if the discovered characteristic belongs to this service */
    if(!DeviceInfoServiceCheckHandle(dev, handle))
    {
        /* Discovered characteristic does not belong this service */
        return FALSE;
    }

    /* Check if the discovered characteristic is supported by this service */
    switch(p_event_data->uuid[0])
    {
        case UUID_DEVICE_INFO_SYSTEM_ID:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_sys_id;
        }
        break;

        case UUID_DEVICE_INFO_MODEL_NUMBER:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_model_num;
        }
        break;

        case UUID_DEVICE_INFO_SERIAL_NUMBER:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_serial_num;
        }
        break;

        case UUID_DEVICE_INFO_HARDWARE_REVISION:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_hw_rev;
        }
        break;

        case UUID_DEVICE_INFO_FIRMWARE_REVISION:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_fw_rev;
        }
        break;

        case UUID_DEVICE_INFO_SOFTWARE_REVISION:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_sw_rev;
        }
        break;

        case UUID_DEVICE_INFO_MANUFACTURER_NAME:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_manufacture_name;
        }
        break;

        case UUID_DEVICE_INFO_PNP_ID:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_pnp_id;
        }
        break;

        case UUID_DEVICE_IEEE_REG_CERTI_DATA_LIST:
        {
            /* Store related values */
            g_dis_data[dev].type[total_char] = dev_info_certi_list;
        }
        break;

        default:
        {
            /* Discovered characteristic is not supported by this service */
            return FALSE;
        }
    }

    /* Store discovered characteristic data */ 
    /* (Example application only supports 16-bit characteristic UUIDs) */
    g_dis_data[dev].chars[total_char].uuid = p_event_data->uuid[0];
    g_dis_data[dev].chars[total_char].valHandle = handle;
    g_dis_data[dev].chars[total_char].properties = p_event_data->prop;
    g_dis_data[dev].chars[total_char].nDescriptors = 0;

    /* Increment total number of supported characteristics discovered on this
     * this Server
     */
    g_dis_data[dev].total_char++;

    return TRUE;
}

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
void DeviceInfoServiceCharDescDisc(uint16 dev,
                                   GATT_CHAR_DESC_INFO_IND_T *p_event_data)
{
    /* Current characteristic */
    const uint16 curr_char = g_dis_data[dev].curr_char - 1;
    /* Number of descriptors discovered for the current characteristic */
    uint8 *numDesc = &g_dis_data[dev].chars[curr_char].nDescriptors;
    /* Properties of the current characteristic */
    const uint8 prop = g_dis_data[dev].chars[curr_char].properties;
    
    /* Only the Client Characteristic Configuration Descriptor is supported by
     * this service
     */
    if(((prop & ATT_PERM_NOTIFY) || (prop & ATT_PERM_INDICATE)) &&
        p_event_data->uuid[0] == UUID_CLIENT_CHAR_CFG)
    {
        /* Add the Client Characteristic Configuration Descriptor 16-bit UUID
         * only. This may be expanded to include support for 128-bit UUIDs.
         */
        g_dis_data[dev].chars[curr_char].descriptors[*numDesc].uuid =
                                            p_event_data->uuid[0];
        g_dis_data[dev].chars[curr_char].descriptors[*numDesc].handle =
                                            p_event_data->desc_handle;

        /* Increment the number of descriptors discovered for the current
         * characteristic
         */
        (*numDesc) ++;
    }
}

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
bool DeviceInfoServiceDiscoveryComplete(uint16 dev, uint16 connect_handle)
{
    /* Reset the current characteristic index */
    g_dis_data[dev].curr_char = 0;

    /* Store the connection handle */
    g_dis_data[dev].connect_handle = connect_handle;

    return FALSE;
}

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
bool DeviceInfoServiceReadRequest(uint16 dev, uint16 type)
{
    /* Type is promoted to the service characteristics enumerated type */
    uint16 value_handle;        /* Handle of characteristic value */
    uint16 count;               /* Index into chars array of characteristic
                                 * corresponding to Type
                                 */
    uint16 status;              /* Function status */

    /* Check if read access is permitted to the specified characteristic on
     * this device
     */
    if(!deviceInfoCheckATTPermission(dev, type, ATT_PERM_READ, &count))
    {
        return FALSE;
    }

    /* Check that the characteristic is supported by this service */
    value_handle = g_dis_data[dev].chars[count].valHandle;
    if (value_handle == INVALID_ATT_HANDLE)
    {
        return FALSE;
    }

    /* Update the current characteristic index */
    g_dis_data[dev].curr_char = count;

    /* Send the read request to the Server */
    
    /* Note that the maximum amount of data that may be transmitted in one PDU
     * is limited to 22 octets (DEFAULT_ATT_MTU - 1) so any values longer than
     * this will be truncated. See GattReadLongCharValue() in the Firmware API
     * for a method to read longer values using multiple PDUs.
     */
    status = GattReadCharValue(g_dis_data[dev].connect_handle, value_handle);

    return (status == sys_status_success) ? TRUE : FALSE;
}

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
void DeviceInfoServiceReadConfirm(uint16 dev,
                                  uint16 size,
                                  uint8 *value)
{
    uint16 uuid;            /* UUID of current characteristic */

    /* Take action depending on the characteristic value read. */
    /* This example application prints the characteristic UUID and the value
     * received from the GATT Server.
     */
    
    /* Find the UUID of the characteristic that was read */
    switch(g_dis_data[dev].type[g_dis_data[dev].curr_char])
    {
        case dev_info_manufacture_name: /* Manufacturer Name String */
        {
            uuid = UUID_DEVICE_INFO_MANUFACTURER_NAME;
        }
        break; 
                
        case dev_info_model_num: /* Model Number String */
        {
            uuid = UUID_DEVICE_INFO_MODEL_NUMBER;
        }
        break;

        case dev_info_serial_num: /* Serial Number String */
        {
            uuid = UUID_DEVICE_INFO_SERIAL_NUMBER;
        }
        break;

        case dev_info_hw_rev: /* Hardware Revision String */
        {
            uuid = UUID_DEVICE_INFO_HARDWARE_REVISION;
        }
        break;
                   
        case dev_info_fw_rev: /* Firmware Revision String */
        {
            uuid = UUID_DEVICE_INFO_FIRMWARE_REVISION;
        }
        break;

        case dev_info_sw_rev: /* Software Revision String */
        {
            uuid = UUID_DEVICE_INFO_SOFTWARE_REVISION;
        }
        break;

        case dev_info_sys_id: /* System ID */
        {
            uuid = UUID_DEVICE_INFO_SYSTEM_ID;
        }
        break;
                   
        case dev_info_certi_list: /* IEEE 11073-20601 Regulatory 
                                   * Certification Data List 
                                   */
        {
            uuid = UUID_DEVICE_IEEE_REG_CERTI_DATA_LIST;
        }
        break;

        case dev_info_pnp_id: /* PnP ID */
        {
            uuid = UUID_DEVICE_INFO_PNP_ID;
        }
        break;

        default:
        {
            /* Do Nothing */
            return;
        }
        break;
    }
    
    /* Display the characteristic UUID */
    DebugIfWriteString("\r\n[Read] DIS char UUID = 0x");
    DebugIfWriteUint16(uuid);

    /* Display the characteristic value received from the GATT Server. */

    /* Note that the maximum amount of data that may be transmitted in one PDU
     * is limited to 22 octets (DEFAULT_ATT_MTU - 1) so any values longer than
     * this will be truncated. See GattReadLongCharValue() in the Firmware API
     * for a method to read longer values using multiple PDUs.
     */
    DebugIfWriteString("\r\n               Value = 0x");
    for (;size > 0;size--)
    {
        DebugIfWriteUint8(value[size - 1]);
    }
}

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
bool DeviceInfoServiceFound(uint16 dev)
{
    if((g_dis_data[dev].service_start_handle != INVALID_ATT_HANDLE) &&
       (g_dis_data[dev].service_end_handle != INVALID_ATT_HANDLE))
    {
        /* Service is supported for the specified device */
        return TRUE;
    }

    /* Service has yet to be discovered or is not supported */
    return FALSE;
}

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
void DeviceInfoServiceResetData(uint16 dev)
{
    deviceInfoDataInit(dev);
}
