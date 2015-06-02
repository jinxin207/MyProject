#ifndef __EH_SMART_SERVICE_H__
#define __EH_SMART_SERVICE_H__

/*============================================================================*
 *  SDK Header Files
 *============================================================================*/

#include <types.h>
#include <bt_event_types.h>
#include "gatt_access.h"
#include "gatt_server.h"
/*============================================================================*
 *  Local Header Files
 *============================================================================*/


/*============================================================================*
 *  Public Function Prototypes
 *============================================================================*/
/* Blood Pressure Service data type */
typedef struct
{

    /* Flag for pending indication confirm */
    bool                    ind_cfm_pending;

    /* Client configuration for BP measurement characteristic */
    gatt_client_config      meas_client_config;

    /* Offset at which Blood Pressure data is stored in NVM */
    uint16                  nvm_offset;

} EhSmart_SERV_DATA_T;
/* This function is used to initialise Blood Pressure service data 
 * structure
 */
extern void EhSmartDataInit(void);
/* This function handles read operation on Blood Pressure service 
 * attributes maintained by the application
 */
extern void EhSmartHandleAccessRead(GATT_ACCESS_IND_T *p_ind);

/* This function handles write operation on Blood Pressure service 
 * attributes maintained by the application
 */
extern void EhSmartHandleAccessWrite(GATT_ACCESS_IND_T *p_ind);

/* This function is used to send BP measurement reading as an indication 
 * to the connected host
 */

/* This function is used to register pending confirmation for the 
 * transmitted BP measurement indications
 */
extern void EhSmartRegIndicationCfm(bool ind_state);

/* This function returns the status of pending confirmation for the 
 * transmitted BP measurement indications
 */
extern bool EhSmartIndCfmPending(void);

/* This function is used to read Blood Pressure service specific data 
 * stored in NVM
 */
extern void EhSmartReadDataFromNVM(uint16 *p_offset);

/* This function is used to check if the handle belongs to the Blood 
 * Pressure service
 */
extern bool EhSmartCheckHandleRange(uint16 handle);

/* This function is used by application to notify bonding status to 
 * Blood Pressure service
 */
extern void EhSmartBondingNotify(void);


#endif	/*__EH_SMART_SERVICE_H__*/