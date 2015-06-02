/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gatt_access.c
 *
 *  DESCRIPTION
 *      GATT-related routines implementations
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <ls_app_if.h>      /* Link Supervisor application interface */
#include <gap_app_if.h>     /* GAP application interface */
#include <gap_types.h>      /* GAP definitions */
#include <ls_err.h>         /* Upper Stack Link Supervisor error codes */
#include <ls_types.h>       /* Link Supervisor definitions */
#include <panic.h>          /* Support for applications to panic */
#include <gatt.h>           /* GATT application interface */
#include <gatt_uuid.h>      /* Common Bluetooth UUIDs and macros */
#include <mem.h>            /* Memory library */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "gatt_client.h"    /* Interface to top level application functions */
#include "gatt_access.h"    /* Interface to this file */
#include "debug_interface.h"/* Debug routines */

/*============================================================================*
 *  Private Data types
 *============================================================================*/

/* GATT data structure */
typedef struct _APP_GATT_DATA_T
{
	/* Stores all the supported services found on the Server */
    SERVICE_FUNC_POINTERS_T     *serviceStore[MAX_SUPPORTED_SERVICES];

    /* Number of supported services found on the Server (number of entries in
     * the serviceStore array)
     */
    uint16                      totalSupportedServices;

    /* Index into serviceStore of service currently being discovered, or
     * configured
     */
    uint16                      currentServiceIndex;
    
    /* Pointer to a service in serviceStore for which a char read request has
     * been sent
     */
    SERVICE_FUNC_POINTERS_T     *read_pService;

    /* Pointer to a service in serviceStore for which a char write request has
     * been sent.
     * Not actively used in this example application because no characteristic
     * value write requests are initiated.
     */
    SERVICE_FUNC_POINTERS_T    *write_pService;
    
    /* Flag to indicate whether the discovered service has data pending to
     * read and/or write
     */
    bool                        service_incomplete;

    /* Flag to indicate whether the peer device is being configured. If the
     * state of this flag is changed from TRUE to FALSE, alter the application
     * state to app_state_configured
     */
    bool                       config_in_progress;

    /* Flag to indicate whether the peer device did not allow configuration,
     * either because of gatt_status_insufficient_authentication or
     * gatt_status_insufficient_authorization
     */
    bool                       pairing_in_progress;
    
    /* Flag to indicate that devices should be filtered by the services that
     * they advertise
     */
    bool                       filter_by_service;
} APP_GATT_DATA_T;

/*============================================================================*
 *  Private Data 
 *============================================================================*/

/* Application GATT data instance */
static APP_GATT_DATA_T g_app_gatt_data;

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/

/* Check and handle if there are any other filtering requirements, other than
 * UUID
 */
static bool appGattCheckFilter(DISCOVERED_DEVICE_T *device,
                               LM_EV_ADVERTISING_REPORT_T *p_event_data);

/* Check if the service for which discovery was initiated is mandatory and if so
 * disconnect the device if the service cannot be found.
 */
static bool appGattCheckMandatoryFoundService(void);

/* Discover all the characteristics for a given service */
static void appGattDiscServiceAllChar(uint16 connect_handle);

/* Discover all the characteristic descriptors for a given characteristic */
static bool appGattDiscCharDescriptors(uint16 connect_handle);

/* Notify the current discovered service and check if it initiates any
 * read/write procedures. If not, then start discovering the next service's
 * characteristics
 */
static void appGattNotifyServAndDiscNext(uint16 connect_handle);

/* Configure all the supported characteristics of all the support services */
static void appGattConfigureServices(uint16 dev);

/* --- API for Discovery Procedures --- */

/* Handle the advertising report */
static void appGattSignalLmAdvertisingReport(
                    LM_EV_ADVERTISING_REPORT_T *p_event_data);

/* Handle the GATT service discovery by UUID */
static void appGattSignalGattDiscPrimServByUuidInd(
                    GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data);

/* Handle the end of GATT service discover-by-UUID sub-procedure */
static void appGattSignalGattDiscPrimServByUuidCfm(
                    GATT_DISC_PRIM_SERV_BY_UUID_CFM_T *p_event_data);

/* Handle GATT_CHAR_DECL_INFO_IND messages received from the firmware */
static void appGattSignalGattCharDeclInforInd(
                    GATT_CHAR_DECL_INFO_IND_T *p_event_data);

/* Handle GATT_DISC_SERVICE_CHAR_CFM messages received from the firmware */
static void appGattSignalGattDiscServiceCharCfm(
                    GATT_DISC_SERVICE_CHAR_CFM_T *p_event_data);

/* Handle GATT_CHAR_DESC_INFO_IND messages received from the firmware */
static void appGattSignalGattCharDescInfoInd(
                    GATT_CHAR_DESC_INFO_IND_T *p_event_data);

/* Handle GATT_DISC_ALL_CHAR_DESC_CFM messages received from the firmware */
static void appGattSignalGattDiscAllCharDescCfm(
                    GATT_DISC_ALL_CHAR_DESC_CFM_T *p_event_data);

/* Handle the attribute notification or indication (GATT_CHAR_VAL_IND) */
static void appGattSignalGattCharValInd(GATT_CHAR_VAL_IND_T *p_event_data);

/* Handle GATT_READ_CHAR_VAL_CFM messages received from the firmware */
static void appGattSignalGattReadCharValCfm(
                    GATT_READ_CHAR_VAL_CFM_T *p_event_data);

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattCheckFilter
 *
 *  DESCRIPTION
 *      Check and handle if there are any filtering requirements, other
 *      than UUID.
 *
 *  PARAMETERS
 *      device [in]             Device to check the filter for
 *      p_event_data [in]       Advertising report event data
 *
 *  RETURNS
 *      TRUE if the specified device passes the filtering requirement(s),
 *      otherwise FALSE
 *----------------------------------------------------------------------------*/
static bool appGattCheckFilter(DISCOVERED_DEVICE_T *device,
                               LM_EV_ADVERTISING_REPORT_T *p_event_data)
{
    /* For now no filtering other than UUID is being performed on the data */

    /* Print out the device details */
    DebugIfWriteString("Found device (");
    DebugIfWriteBdAddress(&device->address);
    DebugIfWriteString(") ");
    DebugIfWriteString("\r\n");

    return TRUE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattCheckMandatoryFoundService
 *
 *  DESCRIPTION
 *      Check if the current service being discovered is mandatory, and if so
 *      disconnect the device if the service is not found.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      TRUE if the service is not mandatory, or if the service is mandatory
 *      and has been found, otherwise FALSE.
 *----------------------------------------------------------------------------*/
static bool appGattCheckMandatoryFoundService(void)
{
    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Current service */
    const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    if(pService != NULL)
    {
        if(pService->isMandatory != NULL)
        {
            if(pService->isMandatory()) /* Service is mandatory */
            {
               if(pService->isServiceFound != NULL &&
                  pService->isServiceFound(dev))
                {
                    /* Service is mandatory and has been found */
                    return TRUE;
                }
                else
                {
                    /* Service is mandatory, but either the service has not been
                     * found or isServiceFound is not defined. So stop the
                     * Discovery Procedure and disconnect the device.
                     */
                    DisconnectDevice(dev);
                    return FALSE;
                }
            }
        }
        else
        {
            /* As isMandatory is not defined, this is treated as a non-
             * mandatory service
             */
            return TRUE;
        }
    }

    return TRUE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattDiscServiceAllChar
 *
 *  DESCRIPTION
 *      Start the discovery of all the characteristics of the current service.
 *
 *  PARAMETERS
 *      connect_handle [in]     Connection handle for the current device
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattDiscServiceAllChar(uint16 connect_handle)
{
    uint16 l_start_handl;   /* Start handle of the current service */
    uint16 l_end_handl;     /* End handle of the current service */

    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Current service */
    const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    while(g_app_gatt_data.currentServiceIndex <
          g_app_gatt_data.totalSupportedServices)
    {
        /* Check if the service is found */
        if(pService != NULL && 
           pService->isServiceFound != NULL &&
           pService->isServiceFound(dev) && /* TRUE means service was found */
           pService->getHandles != NULL)
        {
            /* Get the service handles */
            pService->getHandles(dev,
                                 &l_start_handl,
                                 &l_end_handl,
                                 service_type);

            if(l_start_handl != INVALID_ATT_HANDLE && 
               l_end_handl != INVALID_ATT_HANDLE)
            {
                /* Start characteristic discovery */
                if(GattDiscoverServiceChar(connect_handle,
                                        l_start_handl,
                                        l_end_handl,
                                        GATT_UUID_NONE,
                                        NULL) != sys_status_success)
                {
                    ReportPanic(app_panic_characteristic_discovery_failed);
                }
                break;
            }
            else
            {
                /* Service not initialised */
            }
        }
        
        /* Point to the next service in serviceStore */
        g_app_gatt_data.currentServiceIndex ++;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattDiscCharDescriptors
 *
 *  DESCRIPTION
 *      Start the discovery of all the characteristic descriptors for all the
 *      characteristics of the current service.
 *
 *  PARAMETERS
 *      connect_handle [in]     Connection handle for the current device
 *
 *  RETURNS
 *      TRUE if the characteristic descriptor discovery procedure is started
 *      FALSE if there are no more characteristics to discover descriptors for
 *----------------------------------------------------------------------------*/
static bool appGattDiscCharDescriptors(uint16 connect_handle)
{
    uint16 l_start_handl;       /* Start handle */
    uint16 l_end_handl;         /* End handle */
    uint16 service_end_hndl;    /* End handle of the current service */

    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Current service */
    const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    if(pService != NULL && /* To avoid crash */
       pService->isServiceFound != NULL && 
       pService->isServiceFound(dev) &&
       pService->getHandles != NULL)
    {
        /* Get the service handles */
        pService->getHandles(dev,
                            &l_start_handl,
                            &l_end_handl,
                            service_type);

        service_end_hndl = l_end_handl;

        /* Initiate the characteristic descriptor discovery procedure for each
         * characteristic with descriptors for the current service.
         */
        while(1)
        {
            /* Get the characteristic handles. This function returns TRUE if
             * there are more characteristics to be checked.
             */
            const bool result = pService->getHandles(dev,
                                 &l_start_handl,
                                 &l_end_handl,
                                 characteristic_type);
            
            if(l_start_handl <= l_end_handl && result)
            {
                if(GattDiscoverAllCharDescriptors(connect_handle,
                                          l_start_handl,
                                          l_end_handl) == sys_status_success)
                {
                    return TRUE;
                }
                else
                {
                    ReportPanic(app_panic_char_desc_disc_failed);
                }
            }
            else if(service_end_hndl == l_end_handl || !result)
            {
                return FALSE;
            }
        }
    }
    return FALSE;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattNotifyServAndDiscNext
 *
 *  DESCRIPTION
 *      Notify the current service that the Discovery Procedure for this service
 *      is complete and check if it initiates any read/write procedures. If not,
 *      then start discovering the next service's characteristics and
 *      characteristic descriptors.
 *
 *  PARAMETERS
 *      connect_handle [in]     Connection handle for the current device
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattNotifyServAndDiscNext(uint16 connect_handle)
{
    /* Current service index */
    uint16 *index = &g_app_gatt_data.currentServiceIndex;
    /* Current service */
    const SERVICE_FUNC_POINTERS_T *pService =
                                           g_app_gatt_data.serviceStore[*index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    if(pService->discoveryComplete == NULL ||
       !(g_app_gatt_data.service_incomplete = pService->discoveryComplete(dev,
                                                connect_handle)))
    {
        /* Discover characteristics of the next service */
        while(1)
        {
            /* Point to the next service to discover */
            (*index)++;

            /* Check only for the discovered services */
            if(*index < g_app_gatt_data.totalSupportedServices)
            {
               if(pService->isServiceFound != NULL && 
                  pService->isServiceFound(dev))
                {
                    appGattDiscServiceAllChar(connect_handle);
                    break;
                }
            }
            else
            {
                /* Reset currentServiceIndex */
                *index = 0;

               /* Start configuring the peer device */
                g_app_gatt_data.config_in_progress = TRUE;

                appGattConfigureServices(dev);

                break;
            }
        }
    }
    else
    {
        /* Callback initiated GATT Read/Write procedure */
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattConfigureServices
 *
 *  DESCRIPTION
 *      Configure all the characteristics of the current service. When all the
 *      characteristics have been configured move onto the next service.
 *      When all the characteristics of all the supported services have been
 *      configured, the application is notified so that it may move to the
 *      app_state_configured state.
 *
 *  PARAMETERS
 *      dev [in]                Device number
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattConfigureServices(uint16 dev)
{
    /* Current service index */
    uint16 *index = &g_app_gatt_data.currentServiceIndex;
    
    /* Service being configured */
    SERVICE_FUNC_POINTERS_T *pService = NULL;

    while(*index < g_app_gatt_data.totalSupportedServices)
    {
        pService = g_app_gatt_data.serviceStore[*index];

        if(pService != NULL &&
           pService->configureService != NULL)
        {
            if(pService->configureService(dev))
            {
                /* Service has more characteristics to configure - do not
                 * increment the index
                 */
                break;
            }
        }

        /* Point to the next service to be configured */
        (*index)++;
    }

    if(*index == g_app_gatt_data.totalSupportedServices)
    {
        /* All the services have been configured. Notify the Application that
         * the peer device is configured.
         */
        g_app_gatt_data.config_in_progress = FALSE;
        
        /* Reset the currentServiceIndex as this will be used in reading
         * characteristic values
         */
        *index = 0;

        DeviceConfigured(dev);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalLmAdvertisingReport
 *
 *  DESCRIPTION
 *      This function handles the advertising report.
 *
 *  PARAMETERS
 *      p_event_data [in]       Advertising event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalLmAdvertisingReport(
                                       LM_EV_ADVERTISING_REPORT_T *p_event_data)
{
    DISCOVERED_DEVICE_T device;     /* Advertising device */
    bool flag = FALSE;              /* Flag to indicate that the device
                                     * advertised at least one supported service
                                     */

    /* Clear the data */
    MemSet(&device, 0x0000, sizeof(DISCOVERED_DEVICE_T));

    /* Copy the advertising device address into the device structure */
    MemCopy(&device.address.addr, 
            &p_event_data->data.address, 
            sizeof(BD_ADDR_T));
    device.address.type = p_event_data->data.address_type;

    if(g_app_gatt_data.filter_by_service)
    {
        /* If devices are to be filtered based on the UUIDs of advertised
         * services, compare the list of services supported by the Client with the
         * list of services advertised by the device.
         */
        if(g_app_gatt_data.totalSupportedServices)
        {
            uint16 data[ADVSCAN_MAX_PAYLOAD];/* Advertising event data */
            uint16 num_uuid_found;          /* Advertised service loop counter */
            uint16 size;                    /* Advertising report size, in octets */
            uint16 num_uuid_stored;         /* Supported service loop counter */
        
            /* Extract service UUIDs from the advertisement */

            /* GapLsFindAdType returns zero if no data is present for the specified
             * criteria
             */

            /* If it is required to check for 128-bit UUIDs, use
             * AD_TYPE_SERVICE_UUID_128BIT_LIST as the criteria
             */
            size = GapLsFindAdType(&p_event_data->data, 
                                   AD_TYPE_SERVICE_UUID_16BIT_LIST, 
                                   data,
                                   ADVSCAN_MAX_PAYLOAD);
            if(size == 0)
            {
                /* No service UUIDs list found - try single service UUID */
                size = GapLsFindAdType(&p_event_data->data, 
                                       AD_TYPE_SERVICE_UUID_16BIT, 
                                       data,
                                       ADVSCAN_MAX_PAYLOAD);
                if(size == 0)
                {
                    /* No service data found at all */
                    return;
                }
            }

            /* Check the service UUIDs */
            for(num_uuid_found = 0; 
                num_uuid_found < (size / 2) && !flag;
                num_uuid_found++)
            /* sizeof(uint16) / sizeof(uint8) is always 2,
             * size / 2 is used as the firmware returns the number of bytes copied,
             * so when a single 16-bit UUID is copied, FW returns 2 
             */
            {
                for(num_uuid_stored = 0; 
                    num_uuid_stored < g_app_gatt_data.totalSupportedServices &&
                        !flag;
                    num_uuid_stored++)
                {
                    SERVICE_FUNC_POINTERS_T *pService =
                                  g_app_gatt_data.serviceStore[num_uuid_stored];
                    GATT_UUID_T type = GATT_UUID_NONE;
                    uint16      uuid[8];

                    /* Get the service UUID */
                    if(pService != NULL &&
                       pService->serviceUuid != NULL)
                    {
                        pService->serviceUuid(&type, uuid);
                    }

                    /* Compare the 16-bit service UUID */
                    if(type == GATT_UUID16 && data[num_uuid_found] == uuid[0])
                    {
                        /* At least one of the supported services is present */

                        /* Set the flag */
                        flag = TRUE;
                    }
                }
            }
        }
    }
    else
    {
        flag = TRUE;
    }

    if (flag == TRUE)
    {
        g_app_gatt_data.currentServiceIndex = 0;

        /* Perform any additional filtering, based on e.g. device name,
         * Bluetooth Address etc.
         */
        if(appGattCheckFilter(&device, p_event_data))
        {
            /* Notify the application that a compatible device has been found */
            DeviceFound(&device);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattDiscPrimServByUuidInd
 *
 *  DESCRIPTION
 *      This function handles the GATT service discovery.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattDiscPrimServByUuidInd(
                        GATT_DISC_PRIM_SERV_BY_UUID_IND_T *p_event_data)
{
    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Current service */
    SERVICE_FUNC_POINTERS_T *pService = g_app_gatt_data.serviceStore[index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    /* Check if the service was found */
    if(pService != NULL &&
       pService->serviceInit != NULL)
    {
        pService->serviceInit(dev, p_event_data);

        /* Notify the application about the service found */
        NotifyServiceFound(pService);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattDiscPrimServByUuidCfm
 *
 *  DESCRIPTION
 *      This function handles the end of GATT service discover-by-UUID 
 *      sub-procedure.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattDiscPrimServByUuidCfm(
                       GATT_DISC_PRIM_SERV_BY_UUID_CFM_T *p_event_data)
{
    if(p_event_data->result == sys_status_success)
    {
        /* Check if the service was not found but was mandatory, and if so
         * disconnect the connection.
         */
        if(!appGattCheckMandatoryFoundService())
        {
            return;
        }

        DebugIfWriteString(" Complete.");

        /* Prevent an infinite loop when the primary service sought
         * does not exist on the GATT server
         */
        g_app_gatt_data.currentServiceIndex++;

        /* Continue discovering primary services */
        GattDiscoverRemoteDatabase(p_event_data->cid);

        if(g_app_gatt_data.currentServiceIndex ==
          g_app_gatt_data.totalSupportedServices)
        {
            SERVICE_FUNC_POINTERS_T **ppService;
            uint16 totalServices;

            g_app_gatt_data.currentServiceIndex = 0;

            /* Primary service discovery is complete. Initiate the discovery of 
             * characteristics and their descriptors for all the services found
             */
            ppService = GetConnServices(NULL, &totalServices);

            g_app_gatt_data.totalSupportedServices = totalServices;

            /* Store only the relevant services */
            MemCopy(g_app_gatt_data.serviceStore, ppService,
                             totalServices * sizeof(SERVICE_FUNC_POINTERS_T *));

            appGattDiscServiceAllChar(p_event_data->cid);
        }
    }
    else
    {
        ReportPanic(app_panic_primary_service_discovery_failed);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattCharDeclInforInd
 *
 *  DESCRIPTION
 *      This function handles GATT_CHAR_DECL_INFO_IND messages received from the
 *      firmware.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattCharDeclInforInd(
                                        GATT_CHAR_DECL_INFO_IND_T *p_event_data)
{
    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Current service */
    const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    if(pService != NULL &&
       pService->charDiscovered != NULL && 
       pService->charDiscovered(dev, p_event_data))
    {
        /* Valid handle and UUID found in this service */
        return;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattDiscServiceCharCfm
 *
 *  DESCRIPTION
 *      This function handles GATT_DISC_SERVICE_CHAR_CFM messages received from
 *      the firmware.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattDiscServiceCharCfm(
                                     GATT_DISC_SERVICE_CHAR_CFM_T *p_event_data)
{
    /* Get the connected device */
    const uint16 dev = GetConnDevice();

    if(p_event_data->result == sys_status_success)
    {
        /* Start discovering the characteristics' descriptors */
        if(!appGattDiscCharDescriptors(p_event_data->cid))
        {
            /* No more descriptors found in all the characteristics of the
             * current service
             */
            appGattNotifyServAndDiscNext(p_event_data->cid);
        }
    }
    else
    {
        /* Something went wrong. We can't recover, so disconnect. */
        DisconnectDevice(dev);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattCharDescInfoInd
 *
 *  DESCRIPTION
 *      This function handles GATT_CHAR_DESC_INFO_IND messages received from the
 *      firmware.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattCharDescInfoInd(
                                        GATT_CHAR_DESC_INFO_IND_T *p_event_data)
{
    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Current service */
    const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
    /* Device undergoing Discovery Procedure */
    const uint16 dev = GetConnDevice();

    /* Inform the service about the discovered characteristic descriptor and
     * store it
     */
    if(pService->descDiscovered != NULL)
    {
       pService->descDiscovered(dev, p_event_data);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattDiscAllCharDescCfm
 *
 *  DESCRIPTION
 *      This function handles GATT_DISC_ALL_CHAR_DESC_CFM messages received from
 *      the firmware.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattDiscAllCharDescCfm(
                                    GATT_DISC_ALL_CHAR_DESC_CFM_T *p_event_data)
{
    /* Get the connected device */
    const uint16 dev = GetConnDevice();

    if(p_event_data->result == sys_status_success)
    {
        /* Start discovering the descriptors of the next characteristic in the
         * service
         */
        if(!appGattDiscCharDescriptors(p_event_data->cid))
        {
            /* No more descriptors found in all the characteristics of the
             * current service
             */
            appGattNotifyServAndDiscNext(p_event_data->cid);
        }
    }
    else
    {
        /* Something went wrong. We can't recover, so disconnect. */
        DisconnectDevice(dev);
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattCharValInd
 *
 *  DESCRIPTION
 *      This function handles the attribute notification or indication
 *      (GATT_CHAR_VAL_IND).
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattCharValInd(GATT_CHAR_VAL_IND_T *p_event_data)
{
    /* Index into serviceStore */
    uint16 index = 0;
    /* Pointer to service callback table in serviceStore */
    SERVICE_FUNC_POINTERS_T *pService = NULL;
    /* Get the connected device */
    const uint16 dev = GetConnDevice();
    
    while(index < g_app_gatt_data.totalSupportedServices)
    {
        pService = g_app_gatt_data.serviceStore[index];

        if(pService != NULL && 
           pService->isServiceFound != NULL && 
           pService->isServiceFound(dev) && /* Service is discovered */
           pService->checkHandle != NULL) 
        {
            if(pService->checkHandle(dev, p_event_data->handle))
            {
                /* Handle belongs to this service */
                if(pService->serviceIndNotifHandler != NULL)
                {
                    pService->serviceIndNotifHandler(dev,
                                                     p_event_data->handle,
                                                     p_event_data->size_value,
                                                     p_event_data->value);
                }
                break;
            }
        }

        index++;
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattWriteCharValCfm
 *
 *  DESCRIPTION
 *      This function handles GATT_WRITE_CHAR_VAL_CFM messages received from the
 *      firmware.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattWriteCharValCfm(
                                        GATT_WRITE_CHAR_VAL_CFM_T *p_event_data)
{
    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Pointer to service callback table in serviceStore */
    SERVICE_FUNC_POINTERS_T *pService = NULL;
    /* Get the connected device */
    const uint16 dev = GetConnDevice();

    if((p_event_data->result == gatt_status_insufficient_authentication) ||
       (p_event_data->result == gatt_status_insufficient_authorization))
    {
        /* The Server has rejected a request to write a characteristic value
         * because the Client has insufficient authentication and/or
         * authorisation.
         */
#ifdef PAIRING_SUPPORT
        /* Initiate the Pairing Procedure */
        g_app_gatt_data.pairing_in_progress = TRUE;
        StartBonding();
#else
        /* Disconnect the device */
        DisconnectDevice(dev);
#endif /* PAIRING_SUPPORT */
    }
    else if(p_event_data->result == sys_status_success)
    {
        /* Successfully modified a characteristic value */
        if(g_app_gatt_data.service_incomplete) 
        {
            /* This case is executed if the service has enabled a write 
             * request.
             */
            g_app_gatt_data.service_incomplete = FALSE;

            /* Notify current service and initiate the discovery of the next
             * service
             */
            appGattNotifyServAndDiscNext(p_event_data->cid);
        }
        else if((GetState(dev) == app_state_discovering) && 
                (g_app_gatt_data.config_in_progress))
        {
            /* This case is executed during the Discovery Procedure when the
             * service is being configured
             */
            pService = g_app_gatt_data.serviceStore[index];

            /* Confirm that the write request has finished */
            if(pService != NULL &&
               pService->writeConfirm != NULL)
            {
                pService->writeConfirm(dev, p_event_data->cid);
            }

            /* Continue the service configuration */
            appGattConfigureServices(dev);
        }
        else if((GetState(dev) == app_state_configured) && 
                !(g_app_gatt_data.config_in_progress))
        /* Code should never reach here because this example application does
         * not perform any write procedures.
         */
        {
            pService = g_app_gatt_data.write_pService;

            /* Confirm that the write request has finished */
            if(pService != NULL &&
               pService->writeConfirm != NULL)
            {
                pService->writeConfirm(dev, p_event_data->cid);
            }

            /* Reset write_pService to make sure it is not re-used by mistake */
            g_app_gatt_data.write_pService = NULL;

            /* Perform next read/write procedure */
            NextReadWriteProcedure(TRUE);
        }
    }
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      appGattSignalGattReadCharValCfm
 *
 *  DESCRIPTION
 *      This function handles GATT_READ_CHAR_VAL_CFM messages received from the
 *      firmware.
 *
 *  PARAMETERS
 *      p_event_data [in]       Event data
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void appGattSignalGattReadCharValCfm(GATT_READ_CHAR_VAL_CFM_T
                                            *p_event_data)
{
    /* Current service index */
    const uint16 index = g_app_gatt_data.currentServiceIndex;
    /* Pointer to service callback table in serviceStore */
    SERVICE_FUNC_POINTERS_T *pService = NULL;
    /* Get the connected device */
    const uint16 dev = GetConnDevice();

    if((p_event_data->result == gatt_status_insufficient_authentication) ||
       (p_event_data->result == gatt_status_insufficient_authorization))
    {
        /* The Server has rejected a request to read a characteristic value
         * because the Client has insufficient authentication and/or
         * authorisation.
         */
#ifdef PAIRING_SUPPORT
        /* Initiate the Pairing Procedure */
        g_app_gatt_data.pairing_in_progress = TRUE;
        StartBonding();
#else
        /* Disconnect the device */
        DisconnectDevice(dev);
#endif /* PAIRING_SUPPORT */
    }
    else if(p_event_data->result == sys_status_success)
    {
        /* Successfully read a characteristic value */
        if((GetState(dev) == app_state_discovering) && 
           g_app_gatt_data.config_in_progress)
        {
            /* This case is executed during the Discovery Procedure when the
             * service is being configured
             */
            pService = g_app_gatt_data.serviceStore[index];

            /* Confirm that the read request has finished */
            if(pService != NULL &&
               pService->readConfirm != NULL)
            {
                pService->readConfirm(dev,
                                      p_event_data->size_value,
                                      p_event_data->value);
            }

            /* Continue the service configuration */
            appGattConfigureServices(dev);
        }
        else if((GetState(dev) == app_state_configured) && 
                !g_app_gatt_data.config_in_progress)
        {
            pService = g_app_gatt_data.read_pService;

            /* Confirm that the read request has finished */
            if(pService != NULL &&
               pService->readConfirm != NULL)
            {
                pService->readConfirm(dev,
                                      p_event_data->size_value,
                                      p_event_data->value);
            }

            /* Reset read_pService to make sure it is not re-used by mistake */
            g_app_gatt_data.read_pService = NULL;

            /* Perform next read/write procedure */
            NextReadWriteProcedure(TRUE);
        }
    }
}

/*============================================================================*
 *  Public Function Implementations
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
void InitGattData(void)
{
    /* Initialise GATT data structure */
    g_app_gatt_data.totalSupportedServices  = 0;
    g_app_gatt_data.currentServiceIndex     = 0;
    g_app_gatt_data.service_incomplete      = FALSE;
    g_app_gatt_data.read_pService           = NULL;
    g_app_gatt_data.write_pService          = NULL;
    g_app_gatt_data.pairing_in_progress     = FALSE;
}

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
bool GattIsAddressResolvableRandom(TYPED_BD_ADDR_T *p_addr)
{
    if(p_addr->type != L2CA_RANDOM_ADDR_TYPE || 
      (p_addr->addr.nap & BD_ADDR_NAP_RANDOM_TYPE_MASK)
                                      != BD_ADDR_NAP_RANDOM_TYPE_RESOLVABLE)
    {
        /* This is not a Resolvable Random Address */
        return FALSE;
    }
    
    /* This is a Resolvable Random Address */
    return TRUE;
}

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
void GattStartScan(uint16 num,
                   SERVICE_FUNC_POINTERS_T *serviceStore[],
                   bool filter)
{
    /* Store the device filtering preference */
    g_app_gatt_data.filter_by_service = filter;
    
    /* Initialise the list of services supported by the device */
    g_app_gatt_data.totalSupportedServices = MIN(num, MAX_SUPPORTED_SERVICES);
    if(g_app_gatt_data.totalSupportedServices > 0)
    {
        /* Copy the array of supported services */
        MemCopy(g_app_gatt_data.serviceStore, serviceStore,
                                       g_app_gatt_data.totalSupportedServices *
                                       sizeof(SERVICE_FUNC_POINTERS_T *));
    }

    /* Configure the GAP modes and scan interval */
    if((GapSetMode(gap_role_central, 
               gap_mode_discover_no, 
               gap_mode_connect_no, 
               gap_mode_bond_yes, 
               gap_mode_security_unauthenticate)
         ) != ls_err_none || 
         (GapSetScanInterval(SCAN_INTERVAL * MILLISECOND, 
                             SCAN_WINDOW * MILLISECOND) != ls_err_none)
       ) 
    {
        ReportPanic(app_panic_set_scan_params);
    }

    /* Select active scanning */
    GapSetScanType(ls_scan_type_active);

    /* Start scanning */
    LsStartStopScan(TRUE,
                    /* Whitelist is not used with the Limited Discovery
                     * or General Discovery procedures of the Central role
                     */
                    whitelist_disabled, 
                    ls_addr_type_public); /* Scan using public address */
    
    /* Wait until a LM_EV_ADVERTISING_REPORT event is received */
}

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
bool GattDiscoverRemoteDatabase(uint16 connect_handle)
{
    bool flag = FALSE;   /* Function status */

    /* Get the connected device */
    const uint16 dev = GetConnDevice();

    while(g_app_gatt_data.currentServiceIndex < 
          g_app_gatt_data.totalSupportedServices)
    {
        /* Current service index */
        const uint16 index = g_app_gatt_data.currentServiceIndex;
        /* Current service */
        const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
        
        /* Check if the service was found */
        if(pService != NULL && 
           pService->isServiceFound != NULL && 
           !pService->isServiceFound(dev)) /* Service has not yet been found on
                                            * this device
                                            */
        {            
            if(pService->serviceUuid != NULL)
            {
                GATT_UUID_T type;           /* UUID type */
                uint16      uuid[8];        /* UUID of service to find */
                
                /* Get the UUID and its type */
                pService->serviceUuid(&type, uuid);

                /* Start the discover service by UUID procedure */
                if(GattDiscoverPrimaryServiceByUuid(connect_handle, 
                                                    type,
                                                    uuid) == sys_status_success)
                {
                    DebugIfWriteString("\r\nFinding service - 0x");

                    /* Printing only 16-bit UUIDs for now */
                    DebugIfWriteUint16(uuid[0]); 

                    flag = TRUE;
                }

                /* Service was found */
                break;
            }
        }
        
        /* Check for the next service */
        g_app_gatt_data.currentServiceIndex ++;
    }

    return flag;
}

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
void GattResetAllServices(uint16 dev)
{
    uint16 index;               /* Loop counter */
    
    for (index=0; index < g_app_gatt_data.totalSupportedServices; index++)
    {
        /* Current service - to improve readability */
        const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];

        /* If the service is supported on the supplied device call the
         * service's data reset callback function
         */
        if(pService != NULL && 
           pService->resetServiceData != NULL)
        {
            pService->resetServiceData(dev);
        }
        else
        {
            ReportPanic(app_panic_service_reset_fail);
        }
    }
}

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
bool GattServiceIncomplete(void)
{
    return g_app_gatt_data.service_incomplete;
}

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
bool GattPairingInitiated(void)
{
    return g_app_gatt_data.pairing_in_progress;
}

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
void GattInitServiceCompletion(uint16 dev, uint16 connect_handle)
{
    if(g_app_gatt_data.service_incomplete)
    {
        /* Current service index */
        const uint16 index = g_app_gatt_data.currentServiceIndex;
        /* Current service */
        const SERVICE_FUNC_POINTERS_T *pService =
                                            g_app_gatt_data.serviceStore[index];
    
        if(pService != NULL &&
           pService->discoveryComplete != NULL)
        {
            g_app_gatt_data.service_incomplete = 
                    pService->discoveryComplete(dev, connect_handle);
        }
    }
}

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
void GattInitiateProcedureAgain(uint16 dev)
{
    if(g_app_gatt_data.pairing_in_progress)
    {
        if(g_app_gatt_data.config_in_progress)
        {
            /* The error was reported during configuration */
            appGattConfigureServices(dev);
        }
        else if(g_app_gatt_data.write_pService ||
                g_app_gatt_data.read_pService)
        {
            /* The error was reported while reading or writing a
             * characteristic value
             */
            NextReadWriteProcedure(FALSE);
        }

        /* Indicate that the Pairing Procedure has completed */
        g_app_gatt_data.pairing_in_progress = FALSE;
    }
}

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
bool GattReadRequest(uint16 dev, 
                     SERVICE_FUNC_POINTERS_T *pService,
                     uint16 char_type)
{
    if(pService != NULL && 
       pService->readRequest != NULL)
    {
        if(pService->readRequest(dev, char_type))
        {
            g_app_gatt_data.read_pService = pService;
            return TRUE;
        }
        else
        {
            /* Something went wrong or this characteristic does not belong to
             * this service
             */
            return FALSE;
        }
    }

    return FALSE;
}

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
SERVICE_FUNC_POINTERS_T *GattFindServiceByUuid(GATT_UUID_T uuid_type, 
                                               const uint16 uuid[])
{
    uint16 index;                   /* Loop counter */

    for (index = 0; index < g_app_gatt_data.totalSupportedServices; index++)
    {
        /* Pointer to current service */
        SERVICE_FUNC_POINTERS_T *pService = g_app_gatt_data.serviceStore[index];
        
        if(pService != NULL &&
           pService->serviceUuid != NULL)
        {
            GATT_UUID_T found_uuid_type;    /* Type of UUID found */
            uint16      found_uuid[8];      /* UUID found */
            
            pService->serviceUuid(&found_uuid_type, found_uuid);
            
            if(found_uuid_type == uuid_type)
            {
                switch(uuid_type)
                {
                    case GATT_UUID16:
                    {
                        if(found_uuid[0] == uuid[0])
                        {
                            /* Service found */
                            return pService;                            
                        }
                    }
                    break;
                    
                    case GATT_UUID128:
                    {
                        if(!MemCmp(found_uuid, uuid, 8))
                        {
                            /* Service found */
                            return pService;
                        }
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
    }

    /* Service not found */
    return NULL;
}

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
void GattDiscoveryEvent(lm_event_code event_code, LM_EVENT_T *p_event_data)
{
    switch(event_code)
    {
        case LM_EV_ADVERTISING_REPORT:
        {
            /* This event is raised when an advertisement or scan response is
             * received
             */
            appGattSignalLmAdvertisingReport(
                                    (LM_EV_ADVERTISING_REPORT_T *)p_event_data);
        }
        break;

        case GATT_DISC_PRIM_SERV_BY_UUID_IND:
        {
            /* Zero or more of these events may be raised after a call
             * to GattDiscoverPrimaryServiceByUuid() and before the 
             * GATT_PRIM_SERV_BY_UUID_CFM event 
             */
            appGattSignalGattDiscPrimServByUuidInd(
                             (GATT_DISC_PRIM_SERV_BY_UUID_IND_T *)p_event_data);
        }
        break;

        case GATT_DISC_PRIM_SERV_BY_UUID_CFM:
        {
            /* Indicates the Discover Primary Service by Service UUID 
             * sub-procedure has completed. This event is raised after a call
             * to GattDiscoverPrimaryServiceByUuid() to indicate that all 
             * discovered services have been reported by 
             * GATT_DISC_PRIM_SERV_BY_UUID_IND events
             */
            appGattSignalGattDiscPrimServByUuidCfm(
                             (GATT_DISC_PRIM_SERV_BY_UUID_CFM_T *)p_event_data);
        }
        break;

        case GATT_CHAR_DECL_INFO_IND:
        {
            /* Lists characteristics discovered through the characteristic 
             * discovery procedures. Zero or more of these events may be raised 
             * after a call to GattDiscoverServiceChar() and before the 
             * GATT_DISC_SERVICE_CHAR_CFM event
             */
            appGattSignalGattCharDeclInforInd(
                                     (GATT_CHAR_DECL_INFO_IND_T *)p_event_data);
        }
        break;

        case GATT_DISC_SERVICE_CHAR_CFM:
        {
            /* Indicates the Discover All Characteristics of a Service 
             * sub-procedure or the Discover Characteristics by UUID 
             * sub-procedure has completed. Service characteristics are 
             * returned in GATT_CHAR_DECL_INFO_IND events
             */
            appGattSignalGattDiscServiceCharCfm(
                                  (GATT_DISC_SERVICE_CHAR_CFM_T *)p_event_data);
        }
        break;

        case GATT_CHAR_DESC_INFO_IND:
        {
            /* Lists characteristic descriptors discovered through
             * Discover All Characteristic Descriptors sub-procedure. 
             * Zero or more of these events may be raised after a call to
             * GattDiscoverAllCharDescriptors() and before the 
             * GATT_DISC_ALL_CHAR_DESC_CFM event
             */
            appGattSignalGattCharDescInfoInd(
                                     (GATT_CHAR_DESC_INFO_IND_T *)p_event_data);
        }
        break;

        case GATT_DISC_ALL_CHAR_DESC_CFM:
        {
            /* Indicates the Discover All Characteristic Descriptors
             * sub-procedure of Characteristic Descriptor Discovery 
             * has completed. Characteristic Descriptors are 
             * returned in GATT_CHAR_DESC_INFO_IND events
             */
            appGattSignalGattDiscAllCharDescCfm(
                                 (GATT_DISC_ALL_CHAR_DESC_CFM_T *)p_event_data);
        }
        break;

        case GATT_WRITE_CHAR_VAL_CFM:
        {
            /* Indicates that a Characteristic Value Write procedure other 
             * than the Write Long Characteristic Values sub-procedure has 
             * completed. This event is raised after a call to 
             * GattWriteCharValueReq()
             */
            appGattSignalGattWriteCharValCfm(
                                     (GATT_WRITE_CHAR_VAL_CFM_T *)p_event_data);
        }
        break;

        case GATT_READ_CHAR_VAL_CFM:
        {
            /* Contains the characteristic value requested by 
             * GattReadCharValue()
             */
            appGattSignalGattReadCharValCfm(
                                      (GATT_READ_CHAR_VAL_CFM_T *)p_event_data);
        }
        break;

        case GATT_IND_CHAR_VAL_IND:
        {
            /* Indicates the peer device has indicated a characteristic value */
        } /* FALLTHROUGH */
        case GATT_NOT_CHAR_VAL_IND:
        {
            /* Indicates the peer has notified a characteristic value */
            
            /* Incoming notification or indication */
            appGattSignalGattCharValInd((GATT_CHAR_VAL_IND_T *)p_event_data);
        }
        break;

        default:
        {
            /* Do Nothing */
        }
        break;
    }
}
