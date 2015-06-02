/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gatt_access.h
 *
 *  DESCRIPTION
 *      Header definitions for common application attributes and GATT procedures
 *
******************************************************************************/

#ifndef __GATT_ACCESS_H__
#define __GATT_ACCESS_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>          /* Commonly used type definitions */
#include <gatt.h>           /* GATT application interface */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "user_config.h"    /* User configuration */

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Invalid UCID indicating we are not currently connected */
#define GATT_INVALID_UCID                    (0xFFFF)

/* Invalid UUID */
#define GATT_INVALID_UUID                    (0x0000)

/* Invalid Attribute Handle */
#define INVALID_ATT_HANDLE                   (0x0000)


/* Maximum Length of Device Name 
 * Note: Do not increase device name length beyond (DEFAULT_ATT_MTU -3 = 20) 
 * octets as GAP service at the moment doesn't support handling of Prepare 
 * write and Execute write procedures.
 */
#define DEVICE_NAME_MAX_LENGTH               (20)

/* The following macro definition should be included only if a user wants
 * the application to have a static random address.
 */
/* #define USE_STATIC_RANDOM_ADDRESS */

/* This macro defines the maximum number of descriptors present in each
 * characteristic
 */
#define MAX_SUPPORTED_DESCRIPTORS            (2)

/* Scanning parameters: */
/* How long to scan for advertisements, ms */
#define SCAN_WINDOW                          (400)
/* How often to scan for advertisements, ms */
#define SCAN_INTERVAL                        (400)

/*============================================================================*
 *  Public Data Types
 *============================================================================*/

/* ENUMERATORS */

/* GATT Client Characteristic Configuration Value (see Bluetooth Core Spec v4
 * Vol 3, Part G Section 3.3.3.3)
 */
typedef enum
{
    gatt_client_config_none            = 0x0000,
    gatt_client_config_notification    = 0x0001,
    gatt_client_config_indication      = 0x0002,
    gatt_client_config_reserved        = 0xFFF4
} gatt_client_config;

/*  Application defined panic codes */
typedef enum
{
    /* Failure while setting advertisement parameters */
    app_panic_set_advert_params,

    /* Failure while setting advertisement data */
    app_panic_set_advert_data,

    /* Failure while setting scan response data */
    app_panic_set_scan_rsp_data,

    /* Failure while registering GATT DB with firmware */
    app_panic_db_registration,

    /* Failure while reading NVM */
    app_panic_nvm_read,

    /* Failure while writing NVM */
    app_panic_nvm_write,

    /* Failure while reading Tx Power Level */
    app_panic_read_tx_pwr_level,

    /* Failure while deleting device from whitelist */
    app_panic_delete_whitelist,

    /* Failure while adding device to whitelist */
    app_panic_add_whitelist,

    /* Failure while triggering connection parameter update procedure */
    app_panic_con_param_update,

    /* Event received in an unexpected application state */
    app_panic_invalid_state,

    /* Unexpected beep type */
    app_panic_unexpected_beep_type,

    /* Unsupported UUID */
    app_panic_uuid_not_supported,

    /* Failure while setting scan parameters */
    app_panic_set_scan_params,

    /* Failure while connecting the peer device */
    app_panic_connection_falied,

    /* Failure while discovering the primary service using the discover-by-UUID
     * sub-procedure
     */
    app_panic_primary_service_discovery_failed,
    
    /* Failure while discovering the characteristic of a service */
    app_panic_characteristic_discovery_failed,
    
    /* Failure while discovering the descriptors of a characteristic */
    app_panic_char_desc_disc_failed,

    /* Failure while configuring the peer device */
    app_panic_config_fail,

    /* Failure while resetting the service data, when the peer device was 
     * disconnected 
     */
    app_panic_service_reset_fail
} app_panic_code;

/* Application states */
typedef enum 
{
    /* Application initial state */
    app_state_init = 0,

    /* Scanning for devices */
    app_state_scanning,

    /* Connecting to a device */
    app_state_connecting,

    /* Connection is established */
    app_state_connected,

    /* Discovering GATT Database/Configuring peer */
    app_state_discovering,

    /* Discovery and configuration complete */
    app_state_configured,

    /* Disconnection initiated by the application */
    app_state_disconnecting,
} app_state;

/* This enum is used while calculating the handles for the service 
 * characteristics and descriptors.
 */
typedef enum
{
    service_type,
    characteristic_type,
    descriptor_type
} gatt_profile_hierarchy_t;

/* STRUCTURES */

/* Service callbacks */
typedef struct _SERVICE_FUNC_POINTERS_T
{
    /* Returns the service UUID and its type (16-bit or 128-bit) */
    void (*serviceUuid)(GATT_UUID_T *type, uint16 *uuid);

    /* Returns TRUE if the service is mandatory */
    bool (*isMandatory)(void);

    /* Initialise service data */
    void (*serviceInit)(uint16 dev_num,
                        GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data);

    /* Returns TRUE if the handle is supported by the service */
    bool (*checkHandle)(uint16 dev_num, uint16 handle);
    
    /* Get the start and end handle for the service or characteristic */
    bool (*getHandles)(uint16 dev_num,
                       uint16 *StartHandle, 
                       uint16 *EndHandle,
                       gatt_profile_hierarchy_t type);

    /* Called for each discovered characteristic */
    bool (*charDiscovered)(uint16 dev_num, 
                           GATT_CHAR_DECL_INFO_IND_T *p_event_data);

    /* Called for each discovered characteristic descriptor. This function must
     * only be called after first calling charDiscovered.
     */
    void (*descDiscovered)(uint16 dev_num,
                           GATT_CHAR_DESC_INFO_IND_T *p_event_data);

    /* Called when the Discovery Procedure is complete for the current server.
     * Returns TRUE if the callback starts a read or write, otherwise FALSE
	 */
    bool (*discoveryComplete)(uint16 dev_num,
                              uint16 connect_handle);

    /* Configure notifications for a given characteristic by writing to the
     * Client Characteristic Configuration Descriptor. Returns TRUE, if
     * successful, otherwise FALSE
     */
    bool (*configureServiceNotif)(uint16 dev_num, 
                                  uint16 type, 
                                  uint8 sub_type, 
                                  bool enable);

    /* Callback for handling indications and notifications */
    bool (*serviceIndNotifHandler)(uint16 dev_num,
                                   uint16 handle,
                                   uint16 size,
                                   uint8 *value);

    /* Callback to modify the value of a characteristic on the peer device */
    bool (*writeRequest)(uint16 dev_num,
                         uint16 type,
                         uint8 *data,
                         uint16 size);

    /* Function called after a characteristic value has been successfully
     * written
    */
    void (*writeConfirm)(uint16 dev_num,
                         uint16 connect_handle);

    /* Callback to read the value of a characteristic on the peer device */
    bool (*readRequest)(uint16 dev_num, uint16 type);

    /* Function called when a characteristic value has been successfully read */
    void (*readConfirm)(uint16 dev_num,
                        uint16 size,
                        uint8 *value);

    /* Configure the service to request notifications or indications */
    bool (*configureService)(uint16 dev_num);

    /* Check if the service is present on the specified device */
    bool (*isServiceFound)(uint16 dev_num);
    
    /* Reset the service data */
    void (*resetServiceData)(uint16 dev_num);
} SERVICE_FUNC_POINTERS_T;

/* Discovered Device structure - used once the advertising reports are received 
 */
typedef struct _DISCOVERED_DEVICE_T
{
    /* Device address */
    TYPED_BD_ADDR_T           address;
    
    /* Device name */
    uint8                     deviceName[DEVICE_NAME_MAX_LENGTH];
} DISCOVERED_DEVICE_T;

/* Device structure */
typedef struct _DEVICE_T
{
    /* An array of all the services found on the connected device */
    SERVICE_FUNC_POINTERS_T  *connected_services[MAX_SUPPORTED_SERV_PER_DEVICE];

    /* Total number of supported services on the device (number of entries in
     * connected_services)
     */
    uint16                    totalConnectedServices;

    /* Device address */
    TYPED_BD_ADDR_T           address;

    /* Connection status */
    bool                      connected;

    /* GATT connection handle */
    uint16                    connectHandle;

    /* HCI connection handle */
    hci_connection_handle_t   hciHandle;

    /* Stores if the device was bonded */
    bool                      bonded;

    /* Pairing Key information */
    SM_KEYSET_T               keys;
    
    /* New keys are requested. Set TRUE when the other device was previously
     * bonded, but has since changed its pairing data
     */
    bool                      requestNewKeys;

    /* Set TRUE to request re-pairing if the is bonded */
    bool                      encryptAgain;
    
    /* Application state for the connected device */
    app_state                 state;
} DEVICE_T;

/* Attribute description */
typedef struct _ATTRIBUTE_T
{
    /* 16-bit UUID */
    uint16                    uuid;

    /* Attribute handle */
    uint16                    handle;
} ATTRIBUTE_T;

/* Characteristic */
typedef struct _CHARACTERISTIC_T
{
    /* Characteristic 16-bit UUID */
    uint16                    uuid;

    /* Value handle */
    uint16                    valHandle;
    
    /* Characteristic properties */
    uint8                     properties;
    
    /* Number of characteristic descriptors (number of entries in descriptors
     * array)
     */
    uint8                     nDescriptors;

    /* Array of characteristic descriptors */
    ATTRIBUTE_T               descriptors[MAX_SUPPORTED_DESCRIPTORS];
} CHARACTERISTIC_T;

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      InitGattData
 *
 *  DESCRIPTION
 *      Initialise the application GATT data.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void InitGattData(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattIsAddressResolvableRandom
 *
 *  DESCRIPTION
 *      Check if the specified address is resolvable random or not.
 *
 *  PARAMETERS
 *      p_addr [in]             Address to check
 *
 *  RETURNS
 *      TRUE if the specified address is Resolvable Random Address
 *      FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool GattIsAddressResolvableRandom(TYPED_BD_ADDR_T *p_addr);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattStartScan
 *
 *  DESCRIPTION
 *      Start scanning for devices advertising the supported services. Up to
 *      MAX_SUPPORTED_SERVICES are supported. If "filter" is set to TRUE then
 *      the Client will not connect to devices that do not advertise any of the
 *      services listed in "serviceStore".
 *
 *  PARAMETERS
 *      num [in]                Number of supported services (number of entries
 *                              in serviceStore array)
 *      serviceStore [in]       Array of supported services
 *      filter [in]             TRUE to filter out devices that do not advertise
 *                              any of the supported services
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattStartScan(uint16 num,
                          SERVICE_FUNC_POINTERS_T *serviceStore[],
                          bool filter);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattDiscoverRemoteDatabase
 *
 *  DESCRIPTION
 *      Start GATT Database discovery on a specified connection.
 *
 *  PARAMETERS
 *      connect_handle [in]     Handle of connection to device with the GATT
 *                              Database to be discovered
 *
 *  RETURNS
 *      TRUE on success, FALSE otherwise
 *----------------------------------------------------------------------------*/
extern bool GattDiscoverRemoteDatabase(uint16 connectHandle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattResetAllServices
 *
 *  DESCRIPTION
 *      Reset all the service data for the device (normally called after a
 *      device has disconnected).
 *
 *  PARAMETERS
 *      dev [in]                Device to reset the service data for
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattResetAllServices(uint16 dev);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattServiceIncomplete
 *
 *  DESCRIPTION
 *      Check whether service discovery is still in progress.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      TRUE if there is a pending service discovery, otherwise FALSE
 *----------------------------------------------------------------------------*/
extern bool GattServiceIncomplete(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattPairingInitiated
 *
 *  DESCRIPTION
 *      Checks whether the Pairing Procedure was initiated by the GATT layer,
 *      because gatt_status_insufficient_authentication or
 *      gatt_status_insufficient_authorization has been received.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      TRUE if the Pairing Procedure is in progress, otherwise FALSE
 *----------------------------------------------------------------------------*/
extern bool GattPairingInitiated(void);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattInitServiceCompletion
 *
 *  DESCRIPTION
 *      Check wether the current service provides a callback function for when
 *      the service discovery is complete, and if so call it.
 *
 *  PARAMETERS
 *      dev [in]                Device undergoing service discovery
 *      connect_handle [in]     Connection handle to the device
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattInitServiceCompletion(uint16 dev, uint16 connect_handle);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattInitiateProcedureAgain
 *
 *  DESCRIPTION
 *      This function is called when pairing is complete after an insuffient 
 *      authorisation/authentication error was reported to the application. It
 *      re-starts what the application was doing when the error was reported.
 *
 *  PARAMETERS
 *      dev [in]                Paired device
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattInitiateProcedureAgain(uint16 dev);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattReadRequest
 *
 *  DESCRIPTION
 *      Issue a read characteristic value request.
 *
 *  PARAMETERS
 *      dev [in]                Device to change characteristic value on
 *      pService [in]           Service supplying characteristic
 *      char_type [in]          Characteristic to change value for
 *
 *  RETURNS
 *      TRUE if the request was successfully sent, otherwise FALSE
 *----------------------------------------------------------------------------*/
extern bool GattReadRequest(uint16 dev, 
                            SERVICE_FUNC_POINTERS_T *pService,
                            uint16 char_type);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattFindServiceByUuid
 *
 *  DESCRIPTION
 *      Find the service in serviceStore with the specified UUID.
 *
 *  PARAMETERS
 *      uuid_type [in]          Type of UUID to find
 *      uuid [in]               UUID to find
 *
 *  RETURNS
 *      Pointer to the service in serviceStore if found, otherwise NULL
 *----------------------------------------------------------------------------*/
extern SERVICE_FUNC_POINTERS_T *GattFindServiceByUuid(GATT_UUID_T uuid_type, 
                                                      const uint16 uuid[]);

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattDiscoveryEvent
 *
 *  DESCRIPTION
 *      Handle all events related to the Discovery Procedure.
 *
 *  PARAMETERS
 *      event_code [in]         Event code
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattDiscoveryEvent(lm_event_code event_code, 
                               LM_EVENT_T *p_event_data);

#endif /* __GATT_ACCESS_H__ */
