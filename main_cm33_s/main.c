/*******************************************************************************
* File Name:   main.c
*
* Description: This is the main file for the main CPU non safe application of
* the code example. It initializes the peripherals, PPCA CPU cores and starts
* it. Then it reads the data shared by the PPCA CPUs and prints.
*
* Related Document: See README.md
*
*
********************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/

#include "cy_pdl.h"
#include "cycfg.h"
#include "cy_system_ppca_init.h"
#include "cybsp.h"
#include <stdio.h>
#include "cy_retarget_io.h"

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Debug UART variables */
static cy_stc_scb_uart_context_t    DEBUG_UART_context; /* DEBUG_UART context */
static mtb_hal_uart_t               DEBUG_UART_hal_obj; /* Debug DEBUG_UART HAL object */

/******************************************************************************
* Macros
*******************************************************************************/
/* These are the addresses where the core0 and core1 images are located. */
#define CORE0_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca0_nvm_C_S_START
#define CORE1_IMAGE_ADDRESS    CYMEM_CM33_0_S_m33s_ppca1_nvm_C_S_START
#define PPCA0_IMAGE_SIZE       CYMEM_CM33_0_S_ppca0_code_SIZE
#define PPCA1_IMAGE_SIZE       CYMEM_CM33_0_S_ppca1_code_SIZE

/* Shared memory addresses for inter-core communication */
/* PPCA cores write to these locations, main core reads from them */
/* Variables located in M4 shared memory space (16KB at 0x20040000-0x20043FFF from PPCA view) */
/* Main core accesses PPCA memory through PPCA peripheral base with memory windows: */
/* M1 (CPU0 data): 0x53020000, M3 (CPU1 data): 0x53040000, M4 (shared): 0x53050000 */
#define PPCA_CPU0_M4_VAR_ADDRESS   0x53050400  /* Written by PPCA Core 0 */
#define PPCA_CPU1_M4_VAR_ADDRESS   0x53050800  /* Written by PPCA Core 1 */


/*******************************************************************************
* Function Prototypes
*******************************************************************************/

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/*******************************************************************************
* Function Name: main
*********************************************************************************
* Summary:
* This is the main function for the non safe project for the main core. It
* performs the initialization of the peripherals, initialization and starting
* of the PPCA CPU cores, and send the data received from PPCA CPU Core 0 through
* UART.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Variable located in the shared memory. */
    int32_t *frequency   = (int32_t *)PPCA_CPU0_M4_VAR_ADDRESS;
    int32_t *new_data    = (int32_t *)PPCA_CPU0_M4_VAR_ADDRESS + 1;
    *frequency = 0;
    *new_data  = 0;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    Cy_SCB_UART_Init(DEBUG_UART_HW, &DEBUG_UART_config, &DEBUG_UART_context);
    Cy_SCB_UART_Enable(DEBUG_UART_HW);

    /* Setup the HAL DEBUG_UART */
    result = mtb_hal_uart_setup(&DEBUG_UART_hal_obj, &DEBUG_UART_hal_config,
                                &DEBUG_UART_context, NULL);

    /* HAL DEBUG_UART init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Initialize redirecting of low level IO */
    result = cy_retarget_io_init(&DEBUG_UART_hal_obj);

    /* retarget IO init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Transmit header to the terminal */
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
    printf("************************************************************\r\n");
    printf("PSOC Control C3M/P8: ADC Filter\r\n");
    printf("************************************************************\r\n\n");
    printf("Connect a wire provided with the kit between AIN1 and AIN1P. \r\n");
    printf("Connect an oscilloscope to the pins AIN1 and AIN0. It should\r\n");
    printf("show source sine from AIN1 and the filtered sine from AIN0. \r\n");
    printf("Rotate the pot R262. If will vary the source frequency. Filter \r\n");
    printf("frequency response can be seen on the oscilloscope.\r\n\n");

    /* enable interrupts */
    __enable_irq();

    /* Initializing and enabling the PPCA Configuration. */
    Cy_PPCA_Enable(PPCA_CNFG);

    /* Enable exclusive access to the EPU resources based
     * on the provided resources allocation configuration. */
    Cy_PPCA_EPU_EnableExclusiveAccess(PPCA_EPU, true);

    /* Initializing ATOP ADC */
    Cy_PPCA_ADC_Init(ADC_SINE_HW, &ADC_SINE_config);
    Cy_PPCA_ADC_Enable(ADC_SINE_HW);

    /* Initializing ATOP ADC */
    Cy_PPCA_ADC_Init(ADC_POT_HW, &ADC_POT_config);
    Cy_PPCA_ADC_Enable(ADC_POT_HW);

    /* Initializing and enabling ATOP Analog reference */
    Cy_PPCA_AREF_Init(AREF_HW, &AREF_config);
    Cy_PPCA_AREF_Enable(AREF_HW);

    /* Initializing ATOP ADC Filter */
    Cy_PPCA_ADC_Filter_Init(ADC_LPF_HW, &ADC_LPF_config);

    /* Initializing and enabling the DAC */
    Cy_PPCA_DAC_Init(DAC_SINE_HW, &DAC_SINE_config);
    Cy_PPCA_DACBUF_Enable(DAC_SINE_HW);

    /* Enable DAC buffer */
    Cy_PPCA_DAC_Enable(DAC_SINE_HW);

    /* Initializing and enabling the DAC */
    Cy_PPCA_DAC_Init(DAC_FILTERED_HW, &DAC_FILTERED_config);
    Cy_PPCA_DACBUF_Enable(DAC_FILTERED_HW);

    /* Enable DAC buffer */
    Cy_PPCA_DAC_Enable(DAC_FILTERED_HW);

    /* Initializing and starting the TCPWM used for triggering ADC is the filter path */
    Cy_TCPWM_PWM_Init(ADC_TRIG_PWM_HW, ADC_TRIG_PWM_NUM, &ADC_TRIG_PWM_config);
    Cy_TCPWM_PWM_Enable(ADC_TRIG_PWM_HW, ADC_TRIG_PWM_NUM);

    /* Initializing and starting the TCPWM used for generating sine wave */
    Cy_TCPWM_PWM_Init(SINE_INTR_PWM_HW, SINE_INTR_PWM_NUM, &SINE_INTR_PWM_config);
    Cy_TCPWM_PWM_Enable(SINE_INTR_PWM_HW, SINE_INTR_PWM_NUM);

    /* Enable EPU */
    Cy_PPCA_EPU_Enable(PPCA_EPU);

    /* Configure EPU Processing Unit used for triggering the  ADC in the filter path  */
    Cy_PPCA_EPU_PU_T1_Configure(ADC_TRIG_PWM_TC_HW, ADC_TRIG_PWM_TC_INDEX, &ADC_TRIG_PWM_TC_put1_config);
    Cy_PPCA_EPU_PU_T1_Enable(ADC_TRIG_PWM_TC_HW, ADC_TRIG_PWM_TC_INDEX, ADC_TRIG_PWM_TC_ENABLE_MODE);

    /* Configure EPU Processing Unit used for generating ADC end of conversion interrupt. */
    Cy_PPCA_EPU_PU_T1_Configure(ADC_EOC_HW, ADC_EOC_INDEX, &ADC_EOC_put1_config);
    Cy_PPCA_EPU_PU_T1_Enable(ADC_EOC_HW, ADC_EOC_INDEX, ADC_EOC_ENABLE_MODE);

    /* Configure EPU Combiner used for triggering the ADC */
    Cy_PPCA_EPU_Combo_Configure(ADC_TRIG_HW, ADC_TRIG_INDEX, &ADC_TRIG_combo_config);

    /* Configure EPU Combiner used for interrupting the PPCA CPU 0 */
    Cy_PPCA_EPU_Combo_Configure(ADC_INTR_HW, ADC_INTR_INDEX, &ADC_INTR_combo_config);

    /* Initializing and starting PPCA CPU Core 0. */
    Cy_System_Init_CPU0((void*)CORE0_IMAGE_ADDRESS, PPCA0_IMAGE_SIZE);

    /* Initializing and starting PPCA CPU Core 1. Use the below line to start PPCA Core 1 */
    /*Cy_System_Init_CPU1((void*)CORE1_IMAGE_ADDRESS, PPCA1_IMAGE_SIZE);*/

    for (;;)
    {
        /* Synchronizing the print with the PPCA CPU interrupt. */
        if(1 == *new_data)
        {
            /* Printing frequency of the output wave */
            printf("Sine wave frequency: %d Hz\r\n",(int)*frequency);
            *new_data = 0;
        }
        Cy_SysLib_Delay(500);
    }
}
