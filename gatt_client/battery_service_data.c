/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      battery_service_data.c
 *
 *  DESCRIPTION
 *      This file keeps information related to Discovered Battery service  
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

#include "battery_service_data.h" /* Interface to this file */
#include "gatt_access.h"    /* GATT-related routines */
#include "debug_interface.h"/* Application debug routines */
#include "gatt_client.h"    /* Interface to top level application functions */

/*============================================================================*
 *  Private Definitions
 *===========================================================================*/

/* Number of characteristics present in this service, range [1, 15] */
#define MAXIMUM_NUMBER_OF_CHARACTERISTIC              (1)

/*============================================================================*
 *  Private Data Types
 *===========================================================================*/

/* Battery service structure used in discovery procedure */
typedef struct _BATTERY_SERVICE_DATA_T
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

    /* Index into chars array of characteristic currently being configured */
    uint16 curr_config_char:4;

    /* Flag set to 1 if configuration is ongoing */
    uint16 config_ongoing:1;

    /* Flag set to 1 if a write request initiated during configuration has
     * been confirmed
     */
    uint16 write_cfm:1;

    /* Flag set to 1 if a read request initiated during configuration has
     * been confirmed
     */
    uint16 read_cfm:1;

    /* Array of characteristic 'types' corresponding to the chars array. Only
     * used in read/write/notify procedures.
     */
    battery_char_t type[MAXIMUM_NUMBER_OF_CHARACTERISTIC];
} BATTERY_SERVICE_DATA_T;

/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Battery Service data.
 * A record is kept for each connected device supporting the service.
 */
static BATTERY_SERVICE_DATA_T g_bs_serv_data[MAX_CONNECTED_DEVICES];

/*============================================================================*
 *  Public Data
 *============================================================================*/

/* Callback function table */
SERVICE_FUNC_POINTERS_T BatteryServiceFuncStore = {
    .serviceUuid            = &BatteryServiceUuid,
    .isMandatory            = NULL,
    .serviceInit            = &BatteryServiceDataInit,
    .checkHandle            = &BatteryServiceCheckHandle,
    .getHandles             = &BatteryServiceGetHandles,
    .charDiscovered         = &BatteryServiceCharDiscovered,
    .descDiscovered         = &BatteryServiceCharDescDisc,
    .discoveryComplete      = &BatteryServiceDiscoveryComplete,
    .serviceIndNotifHandler = &BatteryServiceHandlerNotifInd,
    .configureServiceNotif  = &BatteryServiceConfigNotif,
    .writeRequest           = NULL,
    .writeConfirm           = &BatteryServiceWriteConfirm,
    .readRequest            = &BatteryServiceReadRequest,
    .readConfirm            = &BatteryServiceReadConfirm,
    .configureService       = &BatteryServiceConfigure,
    .isServiceFound         = &BatteryServiceFound,
    .resetServiceData       = &BatteryServiceResetData
};

/*============================================================================*
 *  Private Function Prototypes
 *===========================================================================*/

/* Initialise Battery Service data */
static void batteryDataInit(uint16 dev); 

/* Check if the characteristic supports the specified ATT permissions */
static bool batteryCheckATTPermission(uint16          dev,
                                      battery_char_t  type, 
                                      uint16          permission,
                                      uint16         *count);

/*============================================================================*
 *  Private Function Implementations
 *===========================================================================*/

/*---------------------------------------------------------------------------
 *  NAME
 *      batteryDataInit
 *
 *  DESCRIPTION
 *      Initialise the Battery Service data.
 *
 *  PARAMETERS
 *      dev [in]                Device to initialise service data for
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void batteryDataInit(uint16 dev)
{
    uint16 char_num;            /* Loop counter */
    /* Service data for the specified device */
    BATTERY_SERVICE_DATA_T *data = &g_bs_serv_data[dev];

    /* Reset the data */
    data->connect_handle       = GATT_INVALID_UCID;
    data->service_start_handle = INVALID_ATT_HANDLE;
    data->service_end_handle   = INVALID_ATT_HANDLE;
    data->total_char           = 0; 
    data->curr_char            = 0;
    data->curr_config_char     = 0;
    data->config_ongoing       = FALSE;
    data->write_cfm            = FALSE;
    data->read_cfm             = FALSE;

    /* Reset characteristics array */
    MemSet(data->chars, 0x0, sizeof(data->chars));

    /* Initialise characteristic array values */
    for(char_num = 0; char_num < MAXIMUM_NUMBER_OF_CHARACTERISTIC; char_num ++)
    {
        data->type[char_num]                        = battery_type_invalid; 
        data->chars[char_num].valHandle             = INVALID_ATT_HANDLE;
        data->chars[char_num].descriptors[0].handle = INVALID_ATT_HANDLE;
        data->chars[char_num].descriptors[1].handle = INVALID_ATT_HANDLE;
    }
}

/*---------------------------------------------------------------------------
 *  NAME
 *      batteryCheckATTPermission
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
static bool batteryCheckATTPermission(uint16          dev,
                                      battery_char_t  type, 
                                      uint16          permission,
                                      uint16         *count)
{
    /* Initialise return value */
    *count = 0;

    /* Check the requested characteristic is supported */
    if(type >= battery_type_invalid)
    {
        return FALSE;
    }        

    /* Search for the specified characteristic in the chars array */
    while(g_bs_serv_data[dev].type[*count] != type)
    {
        (*count)++;

        if(*count == MAXIMUM_NUMBER_OF_CHARACTERISTIC)
        {
            /* Characteristic not found */
            return FALSE;
        }
    }

    /* Check that the characteristic supports all the permissions requested */
    if((g_bs_serv_data[dev].chars[*count].properties & permission) != permission) 
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
void BatteryServiceUuid(GATT_UUID_T *type, uint16 *uuid)
{
    *type = GATT_UUID16;
    uuid[0] = UUID_BATTERY_SERVICE;
}

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
void BatteryServiceDataInit(uint16 dev, 
                            GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data)
{
    batteryDataInit(dev);
    
    g_bs_serv_data[dev].service_start_handle = p_event_data->strt_handle;
    g_bs_serv_data[dev].service_end_handle   = p_event_data->end_handle;
}

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
bool BatteryServiceCheckHandle(uint16 dev, uint16 handle)
{
    return (((handle >= g_bs_serv_data[dev].service_start_handle) &&
                    (handle <= g_bs_serv_data[dev].service_end_handle))
                    ? TRUE : FALSE);
}

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
bool BatteryServiceGetHandles(uint16 dev,
                              uint16 *start_hndl,
                              uint16 *end_hndl,
                              gatt_profile_hierarchy_t type)
{
    switch(type)
    {
        case service_type:
        {
            *start_hndl = g_bs_serv_data[dev].service_start_handle;
            *end_hndl   = g_bs_serv_data[dev].service_end_handle;
        }
        break;

        case characteristic_type:
        {
            const uint16 curr_char = g_bs_serv_data[dev].curr_char++;
            
            /* Start handle will be the 'value handle' */
            *start_hndl = g_bs_serv_data[dev].chars[curr_char].valHandle;

            if(curr_char + 1 == g_bs_serv_data[dev].total_char)
            {
                /* If this is the last service characteristic the end handle is
                 * the service end handle.
                 */
                *end_hndl = g_bs_serv_data[dev].service_end_handle;
            }
            else if(curr_char + 1 > g_bs_serv_data[dev].total_char)
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
                *end_hndl =
                         g_bs_serv_data[dev].chars[curr_char + 1].valHandle - 2;
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
bool BatteryServiceCharDiscovered(uint16 dev, 
                                  GATT_CHAR_DECL_INFO_IND_T *p_event_data)
{
    /* Handle of discovered characteristic */
    const uint16 handle = p_event_data->val_handle;

    /* Total number of supported characteristics discovered on this Server */
    const uint16 total_char = g_bs_serv_data[dev].total_char;

    /* Check if the discovered characteristic belongs to this service */
    if(!BatteryServiceCheckHandle(dev, handle))
    {
        /* Discovered characteristic does not belong this service */
        return FALSE;
    }

    /* Check if the discovered characteristic is supported by this service */
    switch(p_event_data->uuid[0])
    {
        case UUID_BATTERY_LEVEL:
        {
            /* Store related values */
            g_bs_serv_data[dev].type[total_char] = battery_level;
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
    g_bs_serv_data[dev].chars[total_char].uuid = p_event_data->uuid[0];
    g_bs_serv_data[dev].chars[total_char].valHandle = handle;
    g_bs_serv_data[dev].chars[total_char].properties = p_event_data->prop;
    g_bs_serv_data[dev].chars[total_char].nDescriptors = 0;

    /* Increment total number of supported characteristics discovered on this
     * this Server
     */
    g_bs_serv_data[dev].total_char++;

    return TRUE;
}

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
void BatteryServiceCharDescDisc(uint16 dev,
                                GATT_CHAR_DESC_INFO_IND_T *p_event_data)
{
    /* Current characteristic */
    const uint16 curr_char = g_bs_serv_data[dev].curr_char - 1;
    /* Number of descriptors discovered for the current characteristic */
    uint8 *numDesc = &g_bs_serv_data[dev].chars[curr_char].nDescriptors;
    /* Properties of the current characteristic */
    const uint8 prop = g_bs_serv_data[dev].chars[curr_char].properties;
    
    /* Only the Client Characteristic Configuration Descriptor is supported by
     * this service
     */
    if(((prop & ATT_PERM_NOTIFY) || (prop & ATT_PERM_INDICATE)) &&
        p_event_data->uuid[0] == UUID_CLIENT_CHAR_CFG)
    {
        /* Add the Client Characteristic Configuration Descriptor 16-bit UUID
         * only. This may be expanded to include support for 128-bit UUIDs.
         */
        g_bs_serv_data[dev].chars[curr_char].descriptors[*numDesc].uuid =
                                            p_event_data->uuid[0];
        g_bs_serv_data[dev].chars[curr_char].descriptors[*numDesc].handle =
                                            p_event_data->desc_handle;

        /* Increment the number of descriptors discovered for the current
         * characteristic
         */
        (*numDesc) ++;
    }
}

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
bool BatteryServiceDiscoveryComplete(uint16 dev, uint16 connect_handle)
{
    /* Reset the current characteristic index */
    g_bs_serv_data[dev].curr_char = 0;

    /* Store the connection handle */
    g_bs_serv_data[dev].connect_handle = connect_handle;

    return FALSE;
}

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
bool BatteryServiceHandlerNotifInd(uint16 dev, 
                                   uint16 handle,
                                   uint16 size,
                                   uint8  *value)
{
    /* Index into chars array of the supplied characteristic */
    uint16 count = 0;

    /* Check the supplied characterstic is belongs to this service */
    if(!BatteryServiceCheckHandle(dev, handle))
    {
        /* Characteristic does not belong to this service */
        return FALSE;
    }

    /* Check whether the supplied characteristic is supported by this service */
    while(g_bs_serv_data[dev].chars[count].valHandle != handle)
    {
        count ++;

        if(count == MAXIMUM_NUMBER_OF_CHARACTERISTIC)
        {
            /* Characteristic is not supported by this service */
            return FALSE;
        }
    }

    /* Act on the notification/indication */
    switch (g_bs_serv_data[dev].type[count])
    {
        case battery_level:
        {
            uint16 size_print;      /* Loop counter */

            DebugIfWriteString("\r\n[Notification] Battery Level\r\n");
            DebugIfWriteString("\r\nValue Handle = 0x");
            DebugIfWriteUint16(handle);

            DebugIfWriteString("    Value = 0x");
            for(size_print = 0; size_print < size; size_print ++)
            {
                DebugIfWriteUint8(value[size - size_print - 1]);
            }
        }
        break;

        default:
        {
            return FALSE;
        }
    }

    return TRUE;
}

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
bool BatteryServiceConfigNotif(uint16 dev,
                               uint16 Type,
                               uint8 SubType,
                               bool Enable)
{
    /* Type is promoted to the service characteristics enumerated type */
    uint16 client_cfg;          /* Descriptor handle corresponding to SubType */
    uint16 count = 0;           /* Index into chars array of characteristic
                                 * corresponding to Type
                                 */
    uint8 notification[2];      /* New descriptor value corresponding to
                                 * Enabled
                                 */

    /* Check that the specified characteristic supports notification on this
     * device. If so, obtain the index into chars of this characteristic.
     */
    /* This example application only supports notifications. Support for
     * indcations may be added by specifying ATT_PERM_INDICATE
     */
    if(!batteryCheckATTPermission(dev, Type, ATT_PERM_NOTIFY, &count))
    {
        return FALSE;
    }

    /* Check if the specified descriptor is supported for this characteristic */
    if(SubType > g_bs_serv_data[dev].chars[count].nDescriptors || !SubType)
    {
        return FALSE;
    }

    /* Obtain the descriptor handle */
    client_cfg =
               g_bs_serv_data[dev].chars[count].descriptors[SubType - 1].handle;
    if (client_cfg == INVALID_ATT_HANDLE)
    {
        return FALSE;
    }

    /* Prepare the new descriptor value based on 'Enable' */
    if(Enable)
    {
        notification[0] = WORD_LSB(gatt_client_config_notification);
        notification[1] = WORD_MSB(gatt_client_config_notification);
    }
    else
    {
        notification[0] = WORD_LSB(gatt_client_config_none);
        notification[1] = WORD_MSB(gatt_client_config_none);
    }

    /* Request that the descriptor be modified */
    GattWriteCharValueReq(g_bs_serv_data[dev].connect_handle, 
                          GATT_WRITE_REQUEST,
                          client_cfg,
                          sizeof(notification),
                          notification);
    
    return TRUE;
}

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
bool BatteryServiceReadRequest(uint16 dev, uint16 type)
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
    if(!batteryCheckATTPermission(dev, type, ATT_PERM_READ, &count))
    {
        return FALSE;
    }

    /* Check that the characteristic is supported by this service */
    value_handle = g_bs_serv_data[dev].chars[count].valHandle;
    if (value_handle == INVALID_ATT_HANDLE)
    {
        return FALSE;
    }

    /* Update the current characteristic index */
    g_bs_serv_data[dev].curr_char = count;

    /* Send the read request to the Server */
    
    /* Note that the maximum amount of data that may be transmitted in one PDU
     * is limited to 22 octets (DEFAULT_ATT_MTU - 1) so any values longer than
     * this will be truncated. See GattReadLongCharValue() in the Firmware API
     * for a method to read longer values using multiple PDUs.
     */
    status = GattReadCharValue(g_bs_serv_data[dev].connect_handle, 
                      value_handle);

    return (status == sys_status_success) ? TRUE : FALSE;
}

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
void BatteryServiceReadConfirm(uint16 dev, 
                               uint16 size,
                               uint8 *value)
{
    if(g_bs_serv_data[dev].config_ongoing)
    {
        /* If the service is being configured, update the read_cfm flag */
        g_bs_serv_data[dev].read_cfm = TRUE;
    }
    else
    {
        /* Take action depending on the characteristic value read. */
        /* This example application does nothing on this service */
        switch(g_bs_serv_data[dev].type[g_bs_serv_data[dev].curr_char])
        {
            case battery_level:
            {
                /* Do nothing */
            }
            break;

            default:
            {
                /* Do Nothing */
            }
            break;
        }
    }
}

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
void BatteryServiceWriteConfirm(uint16 dev, 
                                uint16 connect_handle)
{
    if(g_bs_serv_data[dev].config_ongoing)
    {
        /* If the service is being configured, update the write_cfm flag */
        g_bs_serv_data[dev].write_cfm = TRUE;
    }
    else
    {
        /* Take action depending on the characteristic value written. */
        /* This example application does nothing on this service */
        switch(g_bs_serv_data[dev].type[g_bs_serv_data[dev].curr_char])
        {
            case battery_level:
            {
                /* Do nothing */
            }
            break;

            default:
            {
                /* Do Nothing */
            }
            break;
        }
    }
}

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
bool BatteryServiceConfigure(uint16 dev)
{
    /* Current characteristic type */
    battery_char_t type =
                 g_bs_serv_data[dev].type[g_bs_serv_data[dev].curr_config_char];

    if(g_bs_serv_data[dev].write_cfm)
    {
        /* If the previous write request was successful advance the index to the
         * next characteristic to configure
         */
        g_bs_serv_data[dev].curr_config_char ++;
        
        /* Reset the confirmation flag */
        g_bs_serv_data[dev].write_cfm = FALSE;
    }

    if(type == battery_level)
    {
        /* Configure the Client Configuration Characteristic Descriptor for the
         * current characteristic
         */
        if(!BatteryServiceConfigNotif(dev, type, 0x1, TRUE))
        {
            ReportPanic(app_panic_config_fail);
        }

        g_bs_serv_data[dev].config_ongoing = TRUE;

        return TRUE;
    }

    /* Do not reset curr_char_config, as the configuration is done only once
     * per connection 
     */
    g_bs_serv_data[dev].config_ongoing = FALSE;

    return FALSE;
}

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
bool BatteryServiceFound(uint16 dev)
{
    if((g_bs_serv_data[dev].service_start_handle != INVALID_ATT_HANDLE) &&
       (g_bs_serv_data[dev].service_end_handle != INVALID_ATT_HANDLE))
    {
        /* Service is supported for the specified device */
        return TRUE;
    }

    /* Service has yet to be discovered or is not supported */
    return FALSE;
}

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
void BatteryServiceResetData(uint16 dev)
{
    batteryDataInit(dev);
}
