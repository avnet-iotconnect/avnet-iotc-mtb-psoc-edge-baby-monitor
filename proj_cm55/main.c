/****************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for CM55 CPU
*
* Related Document : See README.md
*
********************************************************************************
* Copyright 2024-2025, Cypress Semiconductor Corporation (an Infineon company) or
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
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"


#include "ipc_communication.h"
//#include "retarget_io_init.h"
#ifdef ML_DEEPCRAFT_CM55
#include "stdlib.h"
#include "audio.h"
#include "baby_cry.h"
#endif /* ML_DEEPCRAFT_CM55 */

/*****************************************************************************
 * Macros
 *****************************************************************************/
#define TASK_NAME                ("CM55 Task")
#define TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 4)
#define TASK_PRIORITY            (configMAX_PRIORITIES - 1)
#define TASK_DELAY_MSEC          (500U)

/* Enabling or disabling a MCWDT requires a wait time of upto 2 CLK_LF cycles  
 * to come into effect. This wait time value will depend on the actual CLK_LF  
 * frequency set by the BSP.
 */
#define LPTIMER_1_WAIT_TIME_USEC            (62U)

/* Define the LPTimer interrupt priority number. '1' implies highest priority. 
 */
#define APP_LPTIMER_INTERRUPT_PRIORITY      (1U)

//ipc
#define CM55_APP_DELAY_MS           (50U)
#define RESET_VAL                   (0U)

/*****************************************************************************
 * Global Variables
 *****************************************************************************/

static mtb_hal_lptimer_t lptimer_obj;

//ipc
CY_SECTION_SHAREDMEM static ipc_msg_t cm55_msg_data;
static volatile uint8_t msg_cmd = RESET_VAL;
static volatile uint32_t ipc_msg = RESET_VAL;

uint32_t cry_detect = 0;
volatile uint8_t data_refresh_flag = 0;

/*****************************************************************************
 * Function Definitions
 *****************************************************************************/

#ifdef ML_DEEPCRAFT_CM55
static cy_rslt_t system_init(void);
static void cm55_ml_deepcraft_init(void);
#endif /* ML_DEEPCRAFT_CM55 */

/*******************************************************************************
 * Function Name: cm55_task
 *******************************************************************************
 * Summary:
 * This is the FreeRTOS task callback function. 
 * It suspends the task allowing the device to enter deep sleep during
 * idle task.
 *
 * Parameters:
 *  void * arg
 *
 * Return:
 *  void
 *
 *******************************************************************************/

static void cm55_task(void * arg)
{
    CY_UNUSED_PARAMETER(arg);
            
    cm55_msg_data.client_id = CM33_IPC_PIPE_CLIENT_ID;
    cm55_msg_data.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP2;
    cm55_msg_data.cmd = IPC_CMD_INIT;
    cm55_msg_data.value = RESET_VAL;

    for (;;)
    {     	
       	#ifdef ML_DEEPCRAFT_CM55
		pdm_data_process();
		if(data_refresh_flag) {
        	//printf("\n..........ai data is %d.\n\n", audio_data);
            cm55_msg_data.value = cry_detect;
            Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR, 
                                         CM55_IPC_PIPE_EP_ADDR, 
                                         (void *) &cm55_msg_data, 0);
            data_refresh_flag = 0;
        } else {
			cm55_msg_data.value = RESET_VAL;
			data_refresh_flag = 0;
		}

		#endif
       	Cy_SysLib_Delay(50);
    	
        //vTaskSuspend(NULL);
    }
}

/*******************************************************************************
* Function Name: lptimer_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for LPTimer instance. 
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void lptimer_interrupt_handler(void)
{
    mtb_hal_lptimer_process_interrupt(&lptimer_obj);
}

/*******************************************************************************
* Function Name: setup_tickless_idle_timer
********************************************************************************
* Summary:
* 1. This function first configures and initializes an interrupt for LPTimer.
* 2. Then it initializes the LPTimer HAL object to be used in the RTOS 
*    tickless idle mode implementation to allow the device enter deep sleep 
*    when idle task runs. LPTIMER_1 instance is configured for CM55 CPU.
* 3. It then passes the LPTimer object to abstraction RTOS library that 
*    implements tickless idle mode
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
static void setup_tickless_idle_timer(void)
{
    /* Interrupt configuration structure for LPTimer */
    cy_stc_sysint_t lptimer_intr_cfg =
    {
        .intrSrc = CYBSP_CM55_LPTIMER_1_IRQ,
        .intrPriority = APP_LPTIMER_INTERRUPT_PRIORITY
    };

    /* Initialize the LPTimer interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = 
                                    Cy_SysInt_Init(&lptimer_intr_cfg, 
                                                    lptimer_interrupt_handler);
    
    /* LPTimer interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
         CY_ASSERT(0);
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(lptimer_intr_cfg.intrSrc);

    /* Initialize the MCWDT block */
    cy_en_mcwdt_status_t mcwdt_init_status = 
                                    Cy_MCWDT_Init(CYBSP_CM55_LPTIMER_1_HW, 
                                                &CYBSP_CM55_LPTIMER_1_config);

    /* MCWDT initialization failed. Stop program execution. */
    if(CY_MCWDT_SUCCESS != mcwdt_init_status)
    {
         CY_ASSERT(0);
    }
  
    /* Enable MCWDT instance */
    Cy_MCWDT_Enable(CYBSP_CM55_LPTIMER_1_HW,
                    CY_MCWDT_CTR_Msk, 
                    LPTIMER_1_WAIT_TIME_USEC);

    /* Setup LPTimer using the HAL object and desired configuration as defined
     * in the device configurator. */
    cy_rslt_t result = mtb_hal_lptimer_setup(&lptimer_obj, 
                                            &CYBSP_CM55_LPTIMER_1_hal_config);
    
    /* LPTimer setup failed. Stop program execution. */
    if(CY_RSLT_SUCCESS != result)
    {
         CY_ASSERT(0);
    }

    /* Pass the LPTimer object to abstraction RTOS library that implements 
     * tickless idle mode 
     */
    cyabs_rtos_set_lptimer(&lptimer_obj);
}

/*******************************************************************************
* Function Name: cm33_msg_callback
********************************************************************************
* Summary:
*  Callback function called when endpoint-2 (CM55) has received a message
*
* Parameters:
*  msg_data: Message data received throuig IPC
*
* Return :
*  void
*
*******************************************************************************/
#if 0
void cm55_msg_callback(uint32_t * msgData)
{
    ipc_msg_t *ipc_recv_msg;

    if (msgData != NULL)
    {
        /* Cast the message received to the IPC structure */
        ipc_recv_msg = (ipc_msg_t *) msgData;

        /* Extract the command to be processed in the main loop */
        ipc_msg = ipc_recv_msg->value;
    }

    //cm55_pipe2_msg_received = true;
}
#endif

/*****************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 * This is the main function for CM55 application. 
 *    1. It initializes the device and board peripherals.
 *    2. It sets up the LPTimer instance for CM55 CPU.
 *    3. It creates the FreeRTOS application task 'cm55_task'
 *    4. It starts the RTOS task scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 *****************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();
    
    /* Setup the LPTimer instance for CM55*/
    setup_tickless_idle_timer();
    
    
    /* Initialize retarget-io middleware */ 
    //init_retarget_io(); //for printf
    
    
    /* Setup IPC communication for CM55*/
    cm55_ipc_communication_setup();

    Cy_SysLib_Delay(CM55_APP_DELAY_MS);

    #ifdef ML_DEEPCRAFT_CM55
    /* If ML_DEEPCRAFT_CPU is set as CM55, start the task */
    cm55_ml_deepcraft_init();
	#endif /* ML_DEEPCRAFT_CM55 */

    /* Create the FreeRTOS Task */
    result = xTaskCreate(cm55_task, TASK_NAME,
                        TASK_STACK_SIZE * 4, NULL,
                        TASK_PRIORITY, NULL);

    if( pdPASS == result )
    {
        /* Start the RTOS Scheduler */
        vTaskStartScheduler();
    }

}


#ifdef ML_DEEPCRAFT_CM55
/*******************************************************************************
* Function Name: system_init
********************************************************************************
* Summary:
*  Initializes the neural network based on the DEEPCRAFT model and the
*  DEEPCRAFT pre-processor and initializes the PDM PCM block.
*
* Parameters:
*  None
*
* Returns:
*  The status of the initialization.
*
*******************************************************************************/
static cy_rslt_t system_init(void)
{
    cy_rslt_t result;

    /* Initialize DEEPCRAFT pre-processing library */
    IMAI_init();

    /* Configure PDM, PDM clocks, and PDM event */
    result = pdm_init();

    return result;
}

/*******************************************************************************
* Function Name: cm55_ml_deepcraft_task
********************************************************************************
* Summary:
*  Contains the main loop for the application. It sets up the UART for
*  logs and initialises the system (DEEPCRAFT pre-processor and PDM/PCM block
*  for audio input). It then invokes the PDM Data Processing function that
*  sends the data for pre-processing, inferencing, and prints in the results
*  when enough data is received.
*
* Parameters:
*  None
*
* Returns:
*  None
*
*******************************************************************************/
static void cm55_ml_deepcraft_init(void)
{
    cy_rslt_t result;

    /* Initialize retarget-io middleware */
    //init_retarget_io();

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    //printf("\x1b[2J\x1b[;H");

    /* Initialize inference engine and sensors */
    result = system_init();

    /* Initialization failed */
    if(CY_RSLT_SUCCESS != result)
    {
        /* Failed to initialize properly */
        // printf("System initialization fail\r\n");
        while(1);
    }

/*
    for (;;)
    {
        // Invoke the PDM Data Processing function that sends the data for
        // pre-processing, inferencing, and print the results when enough data
        // is received.
        pdm_data_process();
    }
    */
}
#endif /* ML_DEEPCRAFT_CM55 */

/* [] END OF FILE */
