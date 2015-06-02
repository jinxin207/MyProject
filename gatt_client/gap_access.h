/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gap_access.h
 *
 *  DESCRIPTION
 *      Header definitions for GAP service
 *
 *****************************************************************************/

#ifndef __GAP_ACCESS_H__
#define __GAP_ACCESS_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <bluetooth.h>      /* Bluetooth specific type definitions */
#include "gap_conn_params.h"/* Connection parameter definitions */

/*============================================================================*
 *  Local Header Files
 *============================================================================*/

/*============================================================================*
 *  Public Function Prototypes
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
extern void GapSetDefaultConnParams(ble_con_params *conn_params);

#endif /* __GAP_ACCESS_H__ */
