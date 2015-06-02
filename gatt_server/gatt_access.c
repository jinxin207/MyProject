/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gatt_access.c
 *
 *  DESCRIPTION
 *      GATT-related routine implementations
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
#include <timer.h>          /* Chip timer functions */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "gatt_server.h"    /* Definitions used throughout the GATT server */
#include "app_gatt_db.h"    /* GATT database definitions */
#include "gatt_access.h"    /* Interface to this file */
#include "appearance.h"     /* Macros for commonly used appearance values */
#include "gap_service.h"    /* GAP Service interface */
#include "eh_smart_service.h"
#include "random.h"
/*============================================================================*
 *  Private Definitions
 *============================================================================*/

/* This constant defines an array that is large enough to hold the advertisement
 * data.
 */
#define MAX_ADV_DATA_LEN                                  (31)

/* Acceptable shortened device name length that can be sent in advertisement 
 * data 
 */
#define SHORTENED_DEV_NAME_LEN                            (8)

/* Length of Tx Power prefixed with 'Tx Power' AD Type */
#define TX_POWER_VALUE_LENGTH                             (2)


uint32 SmartUUID = 0xf0140439;
uint16 SmartGROUP = 0x1101;
uint16 SmartADTYPE = 0x0001;
uint16 SmartDataType = 0x4001;
uint8 SmartData[6] ={0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
uint16 RandSeed = 0;

#define EH_W32_0(x) (x & 0xff)
#define WORD32_4SB(_val)              ( ((_val) & 0xff000000) >> 24 )
#define WORD32_3SB(_val)              ( ((_val) & 0x00ff0000) >> 16 )
#define WORD32_2SB(_val)              ( ((_val) & 0x0000ff00) >> 8 )
#define WORD32_1SB(_val)              ( ((_val) & 0x000000ff) )


/*============================================================================*
 *  Private Data types
 *============================================================================*/

/*============================================================================*
 *  Private Data 
 *============================================================================*/

/*============================================================================*
 *  Private Function Prototypes
 *============================================================================*/
/* Set advertisement parameters */
static void gattSetAdvertParams(uint8 fast_connection);

/*============================================================================*
 *  Private Function Implementations
 *============================================================================*/

static uint8 InitRandData(uint8* buf)
{
	uint8 i = 0;
	RandSeed = Random16();
	buf[i++] = AD_TYPE_SERVICE_UUID_16BIT;
	buf[i++] = WORD_MSB(RandSeed);
	buf[i++] = WORD_LSB(RandSeed);

	return i;
}

static uint8 InitUUID32Data(uint8* buf)
{
	
	uint8 i = 0;

	buf[i++] = AD_TYPE_SERVICE_UUID_32BIT;
	buf[i++] = WORD32_4SB(SmartUUID);
	buf[i++] = WORD32_3SB(SmartUUID);
	buf[i++] = WORD32_2SB(SmartUUID);
	buf[i++] = WORD32_1SB(SmartUUID);

	return i;
}



extern uint8 BuildEhongSmartData(uint8* buf)
{
	uint8 i =0 ;
	buf[i++] = AD_TYPE_SERVICE_UUID_128BIT;			////used for data

	/////smart uuid
	buf[i++] = WORD32_4SB(SmartUUID);
	buf[i++] = WORD32_3SB(SmartUUID);
	buf[i++] = WORD32_2SB(SmartUUID);
	buf[i++] = WORD32_1SB(SmartUUID);

	/////smart adver type
	buf[i++] = WORD_MSB(SmartADTYPE);
	buf[i++] = WORD_LSB(SmartADTYPE);
	
	/////smart group
	buf[i++] = WORD_MSB(SmartGROUP);
	buf[i++] = WORD_LSB(SmartGROUP);

	/////smart DATA TYPE
	buf[i++] = WORD_MSB(SmartDataType);
	buf[i++] = WORD_LSB(SmartDataType);

	/////smart DATA value
	buf[i++] = SmartData[0];
	buf[i++] = SmartData[1];
	buf[i++] = SmartData[2];
	buf[i++] = SmartData[3];
	buf[i++] = SmartData[4];
	buf[i++] = SmartData[5];

	return i;
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      gattSetAdvertParams
 *
 *  DESCRIPTION
 *      This function is used to set advertisement parameters.
 *
 *  PARAMETERS
 *      p_addr [in]             Bonded host address
 *      fast_connection [in]    TRUE:  Fast advertisements
 *                              FALSE: Slow advertisements
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
static void gattSetAdvertParams(uint8 adv_speed)
{
	uint8 advert_data[MAX_ADV_DATA_LEN];/* Advertisement packet */
	uint16 length;                      /* Length of advertisement packet */
	/* Advertisement interval, microseconds */
	uint32 adv_interval_min;
	uint32 adv_interval_max;

	if(adv_speed == 0)
	{
	    adv_interval_min = FAST_INTERVAL_MIN;
	    adv_interval_max = FAST_INTERVAL_MIN;
	}
	else if(adv_speed == 0xff)
	{
		adv_interval_min = SLOW_INTERVAL_MIN;
	    adv_interval_max = SLOW_INTERVAL_MIN;
	}
	else
	{
		uint32 rand = Random32();
		if(rand < FAST_INTERVAL_MIN)
			adv_interval_min = adv_interval_max = FAST_INTERVAL_MIN;
		else if(rand > SLOW_INTERVAL_MIN)
			adv_interval_min = adv_interval_max =  SLOW_INTERVAL_MIN;
		else 
			adv_interval_min = adv_interval_max = rand;
			
	}

	if((GapSetMode(gap_role_peripheral, gap_mode_discover_general,
	                    gap_mode_connect_undirected, 
	                    gap_mode_bond_no,
	                    gap_mode_security_none) != ls_err_none) ||
	   (GapSetAdvInterval(adv_interval_min, adv_interval_max) 
	                    != ls_err_none))
	{
	    ReportPanic(app_panic_set_advert_params);
	}

	/* Reset existing advertising data */
	if(LsStoreAdvScanData(0, NULL, ad_src_advertise) != ls_err_none)
	{
	    ReportPanic(app_panic_set_advert_data);
	}

	/* Reset existing scan response data */
	if(LsStoreAdvScanData(0, NULL, ad_src_scan_rsp) != ls_err_none)
	{
	    ReportPanic(app_panic_set_scan_rsp_data);
	}

	length = BuildEhongSmartData(advert_data);
	 if (LsStoreAdvScanData(length, advert_data, 
	                    ad_src_advertise) != ls_err_none)
	{
	    ReportPanic(app_panic_set_advert_data);
	}

	 length = InitRandData(advert_data);
	 if (LsStoreAdvScanData(length, advert_data, 
	                    ad_src_advertise) != ls_err_none)
	{
	    ReportPanic(app_panic_set_advert_data);
	}

	 length = InitUUID32Data(advert_data);
	 if (LsStoreAdvScanData(length, advert_data, 
	                    ad_src_advertise) != ls_err_none)
	{
	    ReportPanic(app_panic_set_advert_data);
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
 *      This function initialises the application GATT data.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void InitGattData(void)
{
   
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleAccessRead
 *
 *  DESCRIPTION
 *      This function handles read operations on attributes (as received in 
 *      GATT_ACCESS_IND message) maintained by the application and responds
 *      with the GATT_ACCESS_RSP message.
 *
 *  PARAMETERS
 *      p_ind [in]              Data received in GATT_ACCESS_IND message.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void HandleAccessRead(GATT_ACCESS_IND_T *p_ind)
{
    /* For the received attribute handle, check all the services that support 
     * attribute 'Read' operation handled by application.
     */
    /* More services may be added here to support their read operations */
    if(GapCheckHandleRange(p_ind->handle))
    {
        /* Attribute handle belongs to the GAP service */
        GapHandleAccessRead(p_ind);
    }
    else if(EhSmartCheckHandleRange(p_ind->handle))
    {
    	EhSmartHandleAccessRead(p_ind);
    }
    else
    {
        /* Application doesn't support 'Read' operation on received attribute
         * handle, so return 'gatt_status_read_not_permitted' status.
         */
        GattAccessRsp(p_ind->cid, p_ind->handle, 
                      gatt_status_read_not_permitted,
                      0, NULL);
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      HandleAccessWrite
 *
 *  DESCRIPTION
 *      This function handles write operations on attributes (as received in 
 *      GATT_ACCESS_IND message) maintained by the application and responds
 *      with the GATT_ACCESS_RSP message.
 *
 *  PARAMETERS
 *      p_ind [in]              Data received in GATT_ACCESS_IND message.
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void HandleAccessWrite(GATT_ACCESS_IND_T *p_ind)
{
    /* For the received attribute handle, check all the services that support 
     * attribute 'Write' operation handled by application.
     */
    /* More services may be added here to support their write operations */
    if(GapCheckHandleRange(p_ind->handle))
    {
        /* Attribute handle belongs to GAP service */
        GapHandleAccessWrite(p_ind);
    }
    else if(EhSmartCheckHandleRange(p_ind->handle))
    {
    	EhSmartHandleAccessWrite(p_ind);
    }
    else
    {
        /* Application doesn't support 'Write' operation on received  attribute
         * handle, so return 'gatt_status_write_not_permitted' status
         */
        GattAccessRsp(p_ind->cid, p_ind->handle, 
                      gatt_status_write_not_permitted,
                      0, NULL);
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattStartAdverts
 *
 *  DESCRIPTION
 *      This function is used to start undirected advertisements and moves to 
 *      ADVERTISING state.
 *
 *  PARAMETERS
 *      p_addr [in]             Bonded host address
 *      fast_connection [in]    TRUE:  Fast advertisements
 *                              FALSE: Slow advertisements
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattStartAdverts(uint16 time, uint8 advSpd)
{
    /* Variable 'connect_flags' needs to be updated to have peer address type 
     * if Directed advertisements are supported as peer address type will 
     * only be used in that case. We don't support directed advertisements for 
     * this application.
     */
#ifdef USE_STATIC_RANDOM_ADDRESS
    uint16 connect_flags = L2CAP_CONNECTION_SLAVE_UNDIRECTED | 
                           L2CAP_OWN_ADDR_TYPE_RANDOM;
#else
    uint16 connect_flags = L2CAP_CONNECTION_SLAVE_UNDIRECTED | 
                           L2CAP_OWN_ADDR_TYPE_PUBLIC;
#endif /* USE_STATIC_RANDOM_ADDRESS */

    /* Set advertisement parameters */
    gattSetAdvertParams(advSpd);

    /* Start GATT connection in Slave role */
    GattConnectReq(NULL, connect_flags);

    if(time > 0)
    {
    	StartAdvertTimer(time);
    }

}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GetSupported16BitUUIDServiceList
 *
 *  DESCRIPTION
 *      This function prepares the list of supported 16-bit service UUIDs to be 
 *      added to Advertisement data. It also adds the relevant AD Type to the 
 *      start of AD array.
 *
 *  PARAMETERS
 *      p_service_uuid_ad [out] AD Service UUID list
 *
 *  RETURNS
 *      Size of AD Service UUID list
 *----------------------------------------------------------------------------*/
extern uint16 GetSupported16BitUUIDServiceList(uint8 *p_service_uuid_ad)
{
    uint8   size_data = 0;              /* Size of AD Service UUID list */

    /* Add 16-bit UUID for supported main service */
    p_service_uuid_ad[size_data++] = AD_TYPE_SERVICE_UUID_16BIT_LIST;

    /* Add battery service UUID */
    p_service_uuid_ad[size_data++] = WORD_LSB(0x1234);
    p_service_uuid_ad[size_data++] = WORD_MSB(0x1234);

    /* Add all the supported UUIDs in this function*/

    /* Return the size of AD service data. */
    return ((uint16)size_data);

}


extern uint16 GetSupported128BitUUIDServiceList(uint8 *p_service_uuid_ad)
{
    	uint8 i = 0;
	uint8 j = 0;
	//0xea5a0e44227b455b804c132564a20384
	uint16 uuid[8] = {0xF014, 0xEB15, 0x0439, 0x3000, 0xE001, 0x0000, 0x1001, 0xFFFF};
	//uint16 uuid[8] = {0xea5a, 0x0e44, 0x227b, 0x455b, 0x804c, 0x1325, 0x64a2, 0x0384};
	p_service_uuid_ad[i++] = AD_TYPE_SERVICE_UUID_128BIT_LIST;
	for(j=0; j<8; j++)
	{
		p_service_uuid_ad[i++] = WORD_LSB(uuid[7-j]);
		p_service_uuid_ad[i++] = WORD_MSB(uuid[7-j]);
	}
    	return ((uint16)i);
}

/*----------------------------------------------------------------------------*
 *  NAME
 *      GattIsAddressResolvableRandom
 *
 *  DESCRIPTION
 *      This function checks if the address is resolvable random or not.
 *
 *  PARAMETERS
 *      p_addr [in]             Address to check
 *
 *  RETURNS
 *      TRUE if supplied address is a resolvable private address
 *      FALSE if supplied address is non-resolvable private address
 *----------------------------------------------------------------------------*/
extern bool GattIsAddressResolvableRandom(TYPED_BD_ADDR_T *p_addr)
{
    if(p_addr->type != L2CA_RANDOM_ADDR_TYPE || 
      (p_addr->addr.nap & BD_ADDR_NAP_RANDOM_TYPE_MASK)
                                      != BD_ADDR_NAP_RANDOM_TYPE_RESOLVABLE)
    {
        /* This is not a resolvable private address  */
        return FALSE;
    }
    return TRUE;
}


/*----------------------------------------------------------------------------*
 *  NAME
 *      GattStopAdverts
 *
 *  DESCRIPTION
 *      This function is used to stop advertisements.
 *
 *  PARAMETERS
 *      None
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
extern void GattStopAdverts(void)
{
    switch(GetState())
    {
        case app_state_fast_advertising:
        case app_state_slow_advertising:
            /* Stop on-going advertisements */
            GattCancelConnectReq();
        break;

        default:
            /* Ignore timer in remaining states */
        break;
    }
}

static void GattStartScan(bool sc)
{
    /* Configure the GAP modes and scan interval */
    if((GapSetMode(gap_role_central, 
               gap_mode_discover_general, 
               gap_mode_connect_no, 
               gap_mode_bond_yes, 
               gap_mode_security_unauthenticate)
         ) != ls_err_none || 
         (GapSetScanInterval(400 * MILLISECOND, 
                             400 * MILLISECOND) != ls_err_none)
       ) 
    {
        //ReportPanic(app_panic_set_scan_params);
    }

    /* Select active scanning */
    GapSetScanType(ls_scan_type_active);

    /* Start scanning */
    LsStartStopScan(sc,
                    /* Whitelist is not used with the Limited Discovery
                     * or General Discovery procedures of the Central role
                     */
                    whitelist_disabled, 
                    ls_addr_type_public); /* Scan using public address */
    
    /* Wait until a LM_EV_ADVERTISING_REPORT event is received */
}

extern void StartScan(bool sc)
{
/* Start scanning for advertising devices */
	
    GattStartScan(sc);
}


