/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for CM33 Secure Project.
*
* Related Document: See README.md
*
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
#include "cy_pdl.h"
#include "cybsp.h"
#include "security_config.h"

/*****************************************************************************
* Function Name: main
******************************************************************************
* This is the main function for Cortex M33 CPU secure application
* NOTE: CM33 secure project assumes that certain memory and peripheral regions
* will be accessed from Non secure environment by the CM33 NS /CM55 code,
* For such regions MPC and PPC configurations are applied to make it non secure
* Any access to these regions from the secure side recommended to be done before the
* MPC/PPC configuration is applied.Once any memory or peripheral region is marked
* as non secure it cannot be accessed from the secure side using secure aliased address 
* but may be accessed using non secure aliased address
*****************************************************************************/
int main(void)
{
    uint32_t ns_stack;
    funcptr_void NonSecure_ResetHandler;
    cy_rslt_t result;

    /* TrustZone setup */
    TZ_SAU_Setup();

#if defined (__FPU_USED) && (__FPU_USED == 1U) && \
      defined (TZ_FPU_NS_USAGE) && (TZ_FPU_NS_USAGE == 1U)
    /*FPU initialization*/
    initFPU();
#endif

    /* Set up internal routing, pins, and clock-to-peripheral connections */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();


    /*Enables the PD1 Power Domain*/
    Cy_System_EnablePD1();

    /* 
    * Initialize the clock for the APP_MMIO_TCM (512K) peripheral group.
    * This sets up the necessary clock and peripheral routing to ensure 
    * the APP_MMIO_TCM can be correctly accessed and utilized.
    */
    Cy_SysClk_PeriGroupSlaveInit(
        CY_MMIO_CM55_TCM_512K_PERI_NR, 
        CY_MMIO_CM55_TCM_512K_GROUP_NR, 
        CY_MMIO_CM55_TCM_512K_SLAVE_NR, 
        CY_MMIO_CM55_TCM_512K_CLK_HF_NR
    );

    /* 
    * Initialize the clock for the SMIF0 peripheral group.
    * This sets up the necessary clock and peripheral routing to ensure 
    * the SMIF0 can be correctly accessed and utilized.
    */
    Cy_SysClk_PeriGroupSlaveInit(
        CY_MMIO_SMIF0_PERI_NR,
        CY_MMIO_SMIF0_GROUP_NR,
        CY_MMIO_SMIF0_SLAVE_NR,
        CY_MMIO_SMIF0_CLK_HF_NR
    );

    /* Enable SOCMEM */
    Cy_SysEnableSOCMEM(true);

    /* Configure semaphore */
    config_sema();

    /* Configure MPC for NS */
    config_mpc();

    ns_stack = (uint32_t)(*((uint32_t*)CM33_NS_SP_STORE));
    __TZ_set_MSP_NS(ns_stack);

    NonSecure_ResetHandler = (funcptr_void)(*((uint32_t*)CM33_NS_RESET_HANDLER_STORE));

    /* Clear SYSCPU and APPCPU power domain dependency set by boot code */
    cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPU, CY_PD_PDCM_SYSCPU);

    /* Configure PPC for NS */
    config_ppc();

    /* Start non-secure application */
    NonSecure_ResetHandler();

    for (;;)
    {
    }
}

/* [] END OF FILE */

