/******************************************************************************
* File Name:   security_config.h
*
* Description: This is the security configuration applied by the CM33 secure
*              project
*
*******************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef SECURITY_CONFIG_H
#define SECURITY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "partition_ARMCM33.h"
#include "cy_ms_ctl.h"
#include "cy_syspm_pdcm.h"

#define NS_APP_BOOT_ADDR             (CY_CM33_NS_APP_BOOT_ADDR)
#define CM33_NS_SP_STORE             (NS_APP_BOOT_ADDR)
#define CM33_NS_RESET_HANDLER_STORE  (NS_APP_BOOT_ADDR + 4U)

/* Protection context for cm33 */
#define CM33_NS_PC_VALUE             (5U)
#define CM33_S_PC_VALUE              (2U)
#define CY_IPC_EP_CYPIPE_CM33_ADDR   (1U)
#define SMIF_0_CORE_0_DESELECT_DELAY (7U)
#define TIMEOUT_1_MS                 (1000U)

typedef void (*funcptr_void) (void) __attribute__((cmse_nonsecure_call));

void config_ppc(void);
void config_mpc(void);
void config_sema(void);

#ifdef __cplusplus
}
#endif

#endif /* SECURITY_CONFIG_H */

/* [] END OF FILE */
