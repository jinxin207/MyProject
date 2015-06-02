/******************************************************************************
 *  Copyright Cambridge Silicon Radio Limited 2013-2015
 *  Part of CSR uEnergy SDK 2.4.5
 *  Application version 2.4.5.0
 *
 *  FILE
 *      gap_conn_params.h
 *
 *  DESCRIPTION
 *      MACROs for connection and advertisement parameter values
 *
 *****************************************************************************/

#ifndef __GAP_CONN_PARAMS_H__
#define __GAP_CONN_PARAMS_H__

/*============================================================================*
 *  Public Definitions
 *============================================================================*/

/* Maximum number of connection parameter update requests that can be sent when 
 * connected
 */
#define MAX_NUM_CONN_PARAM_UPDATE_REQS      (2)

/* Brackets should not be used around the values of macros used in .db files. 
 * gattdbgen which creates .c and .h files from .db files does not support
 * brackets and will raise syntax errors.
 */

/* Preferred connection parameter values should be within the range specified by
 * the Bluetooth specification.
 */

/* Minimum and maximum connection interval in number of frames */
#define PREFERRED_MAX_CON_INTERVAL          0x0010 /* 20 ms */
#define PREFERRED_MIN_CON_INTERVAL          0x0010 /* 20 ms */

/* Slave latency in number of connection intervals */
#define PREFERRED_SETUP_SLAVE_LATENCY       0x0000 /* 0 conn_intervals */

/* Slave latency once GATT database discovery is complete */
#define PREFERRED_RUNNING_SLAVE_LATENCY     0x003c /* 60 conn_intervals */

/* Supervision timeout in ms = PREFERRED_SUPERVISION_TIMEOUT * 10 ms */
#define PREFERRED_SUPERVISION_TIMEOUT       0x03E8 /* 10 seconds */

#endif /* __GAP_CONN_PARAMS_H__ */
