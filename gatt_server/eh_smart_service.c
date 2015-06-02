/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2012-2014
 *  Part of CSR uEnergy SDK 2.3.0
 *  Application version 2.3.0.0
 *
 *  FILE
 *      bp_service.c
 *
 * DESCRIPTION
 *      This file defines routines for using Blood Pressure service.
 *
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <gatt.h>
#include <gatt_prim.h>
#include <buf_utils.h>


/*============================================================================*
 *  Local Header Files
 *============================================================================*/
#include "gatt_access.h"
#include "nvm_access.h"
#include "app_gatt_db.h"
#include "panic.h"
#include "battery.h"
#include "debug.h"
#include "eh_smart_service.h"
#include "gatt_server.h"
#include "mem.h"
#include "debug_interface.h"/* Application debug routines */
/*============================================================================*
 *  Private Data Declaration
 *============================================================================*/



/*============================================================================*
 *  Private Data
 *============================================================================*/

/* Blood pressure service data structure */
EhSmart_SERV_DATA_T g_EhSmart_serv_data;

/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* Number of words of NVM memory used by Blood Pressure service*/
#define EhSmart_SERVICE_NVM_MEMORY_WORDS      (1)

/* The offset of data being stored in NVM for Blood Pressure service. 
 * This offset is added to Blood Pressure service offset to NVM 
 * region to get the absolute offset at which this data is stored in NVM
 */
#define EhSmart_NVM_MEAS_CLIENT_CONFIG_OFFSET (0)


/* Minimum data length of BP Measurement characteristic value,
 * which shall have Flags (uint8), Systolic (SFLOAT), Diastolic 
 * (SFLOAT) and Mean Arterial Pressure (SFLOAT)
 */
#define EhSmart_MEAS_MIN_DATA_LENGTH          (7)

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartDataInit
 *
 *  DESCRIPTION
 *      This function is used to initialise Blood Pressure service data 
 *      structure.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/





extern void EhSmartDataInit(void)
{
    if(1)
    {
        /* Initialise BP Measurement Client Configuration descriptor
         * only if device is not bonded
         */
        g_EhSmart_serv_data.meas_client_config = gatt_client_config_none;
    }

    g_EhSmart_serv_data.ind_cfm_pending =FALSE;

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartHandleAccessRead
 *
 *  DESCRIPTION
 *      This function handles Read operation on Blood Pressure service 
 *      attributes maintained by the application and responds with the 
 *      GATT_ACCESS_RSP message.
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void EhSmartHandleAccessRead(GATT_ACCESS_IND_T *p_ind)
{
    uint16 length = 0;
    uint8  val[16]; 
    uint8 *p_value = NULL;
    sys_status rc = sys_status_success;
    switch(p_ind->handle)
    {
        case HANDLE_SMART_SENSOR_C_CFG:
        {
            p_value = val;
            length = 2; /* Two Octets */
            BufWriteUint16((uint8 **)&p_value, 
                g_EhSmart_serv_data.meas_client_config);
        }
        break;

	case HANDLE_SMART_SENSOR:		////
		break;

	case HANDLE_SMART_CONTROL:		////
		break;
	
	case HANDLE_SMART_CONFIG:
		break;
	
        default:
        {
            /* Let the firmware handle the request. */
            rc = gatt_status_irq_proceed;
        }
        break;
    }

    GattAccessRsp(p_ind->cid, p_ind->handle, rc,
                  length, val);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartHandleAccessWrite
 *
 *  DESCRIPTION
 *      This function handles write operation on Blood Pressure service 
 *      attributes maintained by the application.and responds with the 
 *      GATT_ACCESS_RSP message.
 *
 *  RETURNS
 *      Nothing
 *
 *---------------------------------------------------------------------------*/

extern void EhSmartHandleAccessWrite(GATT_ACCESS_IND_T *p_ind)
{
    uint8 *p_value = p_ind->value;
//    uint8 len = p_ind->size_value;
    uint16 client_config;
    sys_status rc = sys_status_success;

    switch(p_ind->handle)
    {
        case HANDLE_SMART_SENSOR_C_CFG:
        {
            client_config = BufReadUint16(&p_value);

            /* Client configuration is a bit field value, so ideally bit wise 
             * comparison should be used but since the application supports only
             * notifications or nothing, direct comparison should be used.
             */
            if((client_config == gatt_client_config_notification) ||
               (client_config == gatt_client_config_none))
            {
                g_EhSmart_serv_data.meas_client_config = client_config;

                /* Write BP Measurement Client configuration to NVM if the 
                 * device is bonded.
                 */
            }
            else
            {
                /* Notification or RESERVED */

                /* Return Error as only Indications are supported */
                rc = gatt_status_app_mask;
            }

        }
		break;

	case HANDLE_SMART_CONTROL:		////
		DebugIfWriteString("smart control\r\n");
		break;

	case HANDLE_SMART_CONFIG:
		switch(p_value[0])
		{
			case 0x00:		////adver uuid
				break;
			case 0x01:		////adver interval
				break;
			case 0x02:		////role, scan or adver?, 01: adver, 02: scan , 03: scan & adver 
				break;
			case 0x03:		////group
				break;
			case 0x04:		////data type
				break;
			case 0x05:		////adver type
				break;
			case 0x06:		////
				break;
			default:
				break;
		}
		break;
	
    }

    /* Send ACCESS RESPONSE */
    GattAccessRsp(p_ind->cid, p_ind->handle, rc, 0, NULL);

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartRegIndicationCfm
 *
 *  DESCRIPTION
 *      This function is used to register pending confirmation for the 
 *      transmitted BP measurement indications
 *
 *  RETURNS
 *      Nothing
 *
 *---------------------------------------------------------------------------*/

extern void EhSmartRegIndicationCfm(bool ind_state)
{
    g_EhSmart_serv_data.ind_cfm_pending = ind_state;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartIndCfmPending
 *
 *  DESCRIPTION
 *      This function returns the status of pending confirmation for the 
 *      transmitted BP measurement indications
 *
 *  RETURNS
 *      Boolean: TRUE (Confirmation pending) OR
 *               FALSE (Confirmation not pending)
 *
 *---------------------------------------------------------------------------*/

extern bool EhSmartIndCfmPending(void)
{
    return g_EhSmart_serv_data.ind_cfm_pending;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartReadDataFromNVM
 *
 *  DESCRIPTION
 *      This function is used to read Blood Pressure service specific data 
 *      stored in NVM
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void EhSmartReadDataFromNVM(uint16 *p_offset)
{

    g_EhSmart_serv_data.nvm_offset = *p_offset;

    /* Read NVM only if devices are bonded */
    if(1)
    {
        /* Read BP measurement client configuration */
        Nvm_Read((uint16*)&g_EhSmart_serv_data.meas_client_config,
                 sizeof(g_EhSmart_serv_data.meas_client_config),
                 *p_offset + 
                 EhSmart_NVM_MEAS_CLIENT_CONFIG_OFFSET);

    }

    /* Increment the offset by the number of words of NVM memory required 
     * by Blood Pressure service 
     */
    *p_offset += EhSmart_SERVICE_NVM_MEMORY_WORDS;

}


/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartCheckHandleRange
 *
 *  DESCRIPTION
 *      This function is used to check if the handle belongs to the Blood 
 *      Pressure service
 *
 *  RETURNS
 *      Boolean - Indicating whether handle falls in range or not.
 *
 *---------------------------------------------------------------------------*/

extern bool EhSmartCheckHandleRange(uint16 handle)
{
    return ((handle >= HANDLE_SMART_SERVICE) &&
            (handle <= HANDLE_SMART_SERVICE_END))
            ? TRUE : FALSE;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      EhSmartBondingNotify
 *
 *  DESCRIPTION
 *      This function is used by application to notify bonding status to 
 *      Blood Pressure service
 *
 *  RETURNS
 *      Nothing.
 *
 *---------------------------------------------------------------------------*/

extern void EhSmartBondingNotify(void)
{
    /* Write data to NVM if bond is established */
    if(1)
    {
        /* Write to NVM the client configuration value */
        Nvm_Write((uint16*)&g_EhSmart_serv_data.meas_client_config,
                  sizeof(g_EhSmart_serv_data.meas_client_config),
                  g_EhSmart_serv_data.nvm_offset + 
                  EhSmart_NVM_MEAS_CLIENT_CONFIG_OFFSET);
    }

}

