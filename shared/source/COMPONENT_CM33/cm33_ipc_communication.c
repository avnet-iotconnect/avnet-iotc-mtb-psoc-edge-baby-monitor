/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the code for setting up IPC 
*                    communication for CM33 CPU.
*
* Related Document : See README.md
*
********************************************************************************
* Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
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

#include "ipc_communication.h"


/*******************************************************************************
* Global Variable(s)
*******************************************************************************/
/* Create an array of endpoint structures */
static cy_stc_ipc_pipe_ep_t cm33_ipc_pipe_ep_array[CY_IPC_MAX_ENDPOINTS];

/* CB Array for EP1 */
static cy_ipc_pipe_callback_ptr_t ep1_cb_array[CY_IPC_CYPIPE_CLIENT_CNT]; 

/* Allocate and initialize semaphores for the system operations. */
CY_SECTION_SHAREDMEM
static uint32_t ipc_sema_array[CY_IPC_SEMA_COUNT / CY_IPC_SEMA_PER_WORD];


/*******************************************************************************
* Function Name: cm33_ipc_pipe_isr
********************************************************************************
*
* This is the interrupt service routine for the system pipe.
*
* Parameters:
*  none
*
* Return :
*  void
*
*******************************************************************************/
void cm33_ipc_pipe_isr(void)
{
    Cy_IPC_Pipe_ExecuteCallback(CM33_IPC_PIPE_EP_ADDR);
}


/*******************************************************************************
* Function Name: cm33_ipc_communication_setup
********************************************************************************
* Summary:
* This function...
* 1. Initializes IPC Semaphore.
* 2. Configures IPC Pipe for CM33 to CM55 communication.
*
* Parameters:
*  none
*
* Return :
*  void
*
*******************************************************************************/
void cm33_ipc_communication_setup(void)
{
    /* IPC pipe endpoint-1 and endpoint-2. CM33 <--> CM55 */
    static const cy_stc_ipc_pipe_config_t cm33_ipc_pipe_config =
    {
        /* receiver endpoint CM33 */
        {
            .ipcNotifierNumber    = CY_IPC_INTR_CYPIPE_EP1,
            .ipcNotifierPriority  = CY_IPC_INTR_CYPIPE_PRIOR_EP1,
            .ipcNotifierMuxNumber = CY_IPC_INTR_CYPIPE_MUX_EP1,
            .epAddress            = CM33_IPC_PIPE_EP_ADDR,
            {
                .epChannel        = CY_IPC_CHAN_CYPIPE_EP1,
                .epIntr           = CY_IPC_INTR_CYPIPE_EP1,
                .epIntrmask       = CY_IPC_CYPIPE_INTR_MASK
            }
        },
        /* sender endpoint CM55 */
        {
            .ipcNotifierNumber     = CY_IPC_INTR_CYPIPE_EP2,
            .ipcNotifierPriority   = CY_IPC_INTR_CYPIPE_PRIOR_EP2,
            .ipcNotifierMuxNumber  = CY_IPC_INTR_CYPIPE_MUX_EP2,
            .epAddress             = CM55_IPC_PIPE_EP_ADDR,
            {
                .epChannel         = CY_IPC_CHAN_CYPIPE_EP2,
                .epIntr            = CY_IPC_INTR_CYPIPE_EP2,
                .epIntrmask        = CY_IPC_CYPIPE_INTR_MASK
            }
        },
    .endpointClientsCount          = CY_IPC_CYPIPE_CLIENT_CNT,
    .endpointsCallbacksArray       = ep1_cb_array,
    .userPipeIsrHandler            = &cm33_ipc_pipe_isr
    };

    Cy_IPC_Sema_Init(IPC0_SEMA_CH_NUM, CY_IPC_SEMA_COUNT, ipc_sema_array);

    Cy_IPC_Pipe_Config(cm33_ipc_pipe_ep_array);

    Cy_IPC_Pipe_Init(&cm33_ipc_pipe_config);
}