/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gap_access.c
 *
 *  DESCRIPTION
 *      This file defines routines for using GAP service.
 *
 *****************************************************************************/

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <ls_app_if.h>      /* Link Supervisor application interface */
#include <mem.h>            /* Memory library */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

#include "gatt_access.h"    /* GATT-related routines */
#include "gap_access.h"     /* Interface to this file */
#include "gap_conn_params.h"/* Connection parameters */
#include "gatt_client.h"    /* Interface to top level application functions */

/*============================================================================*
 *  Public Function Implementations
 *============================================================================*/

/*---------------------------------------------------------------------------
 *  NAME
 *      GapSetDefaultConnParams
 *
 *  DESCRIPTION
 *      Set the default connection parameters for new connections.
 *
 *  PARAMETERS
 *      conn_params [in]        New connection parameters, or NULL
 *
 *  RETURNS
 *      Nothing
 *----------------------------------------------------------------------------*/
void GapSetDefaultConnParams(ble_con_params *conn_params)
{
    /* Default connection parameters.
     * Initial values taken from gap_conn_params.h
     */
    ble_con_params default_conn_params = 
                {
                    PREFERRED_MIN_CON_INTERVAL,
                    PREFERRED_MAX_CON_INTERVAL,
                    PREFERRED_SETUP_SLAVE_LATENCY,
                    PREFERRED_SUPERVISION_TIMEOUT
                };

    if(conn_params != NULL)
    {
        MemCopy(&default_conn_params, conn_params, sizeof(ble_con_params));
    }

    if(LsSetNewConnectionParamReq(&default_conn_params,
                                  0,
                                  0,
                                  (uint16)SCAN_INTERVAL,
                                  (uint16)SCAN_WINDOW) != ls_err_none)
    {
        ReportPanic(app_panic_con_param_update);
    }
}

