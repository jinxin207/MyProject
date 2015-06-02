/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gatt_server.h
 *
 *  DESCRIPTION
 *      Header file for a simple GATT server application.
 *
 ******************************************************************************/

#ifndef __GATT_SERVER_H__
#define __GATT_SERVER_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>          /* Commonly used type definitions */
#include <timer.h>          /* Chip timer functions */
/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "gatt_access.h"    /* GATT-related routines */

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Maximum number of words in central device Identity Resolving Key (IRK) */
#define MAX_WORDS_IRK                       (8)

/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/* Application data structure */
typedef struct _APP_DATA_T
{
    /* Current state of application */
    app_state                  state;

    /* TYPED_BD_ADDR_T of the host to which device is connected */
    TYPED_BD_ADDR_T            con_bd_addr;

    /* Track the Connection Identifier (UCID) as Clients connect and
     * disconnect
     */
    uint16                     st_ucid;

    /* Boolean flag to indicate whether the device is bonded */
    bool                       bonded;

    /* TYPED_BD_ADDR_T of the host to which device is bonded */
    TYPED_BD_ADDR_T            bonded_bd_addr;

    /* Diversifier associated with the Long Term Key (LTK) of the bonded
     * device
     */
    uint16                     diversifier;

    /* Timer ID for Connection Parameter Update timer in Connected state */
    timer_id                   con_param_update_tid;

    /* Central Private Address Resolution IRK. Will only be used when
     * central device used resolvable random address.
     */
    uint16                     irk[MAX_WORDS_IRK];

    /* Number of connection parameter update requests made */
    uint8                      num_conn_update_req;

    /* Boolean flag to indicate pairing button press */
    bool                       pairing_button_pressed;

    /* Timer ID for 'UNDIRECTED ADVERTS' and activity on the sensor device like
     * measurements or user intervention in CONNECTED state.
     */
    timer_id                   app_tid;

    /* Boolean flag to indicate whether to set white list with the bonded
     * device. This flag is used in an interim basis while configuring 
     * advertisements.
     */
    bool                       enable_white_list;

    /* Current connection interval */
    uint16                     conn_interval;

    /* Current slave latency */
    uint16                     conn_latency;

    /* Current connection timeout value */
    uint16                     conn_timeout;


	/////application val
	timer_id		role_tid;
} APP_DATA_T;
/* Call the firmware Panic() routine and provide a single point for debugging
 * any application level panics
 */
extern void ReportPanic(app_panic_code panic_code);

/* Handle a short button press. If connected, the device disconnects from the
 * host, otherwise it starts advertising.
 */
extern void HandleShortButtonPress(void);

/* Change the current state of the application */
extern void SetState(app_state new_state);

/* Return the current state of the application.*/
extern app_state GetState(void);

/* Check if the whitelist is enabled or not. */
extern bool IsWhiteListEnabled(void);

/* Handle pairing removal */
extern void HandlePairingRemoval(void);

/* Start the advertisement timer. */
extern void StartAdvertTimer(uint16 interval);

/* Return whether the connected device is bonded or not */
extern bool IsDeviceBonded(void);

/* Return the unique connection ID (UCID) of the connection */
extern uint16 GetConnectionID(void);

#endif /* __GATT_SERVER_H__ */
