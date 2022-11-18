/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the RutDevKit-PSoC62_RTC_Hibernate
*              Application for ModusToolbox.
*
* Related Document: See README.md
*
*
*  Created on: 2022-08-23
*  Company: Rutronik Elektronische Bauelemente GmbH
*  Address: Jonavos g. 30, Kaunas 44262, Lithuania
*  Author: GDR
*
*******************************************************************************
* (c) 2019-2021, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
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
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*
* Rutronik Elektronische Bauelemente GmbH Disclaimer: The evaluation board
* including the software is for testing purposes only and,
* because it has limited functions and limited resilience, is not suitable
* for permanent use under real conditions. If the evaluation board is
* nevertheless used under real conditions, this is done at one’s responsibility;
* any liability of Rutronik is insofar excluded
*******************************************************************************/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#define STRING_BUFFER_SIZE 			80
#define STAY_ONLINE_MS				1000
#define STAY_OFFLINE_SEC			2
#define RTC_INTERRUPT_PRIORITY		5

void handle_error(void);
cy_rslt_t Hibernate(cyhal_rtc_t *obj, uint32_t seconds);

cyhal_rtc_t rtc_obj;

int main(void)
{
    cy_rslt_t result;
    struct tm date_time;
    char buffer[STRING_BUFFER_SIZE];

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
    	handle_error();
    }

    __enable_irq();

    /*Initialize LEDs*/
    result = cyhal_gpio_init( LED1, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    if (result != CY_RSLT_SUCCESS)
    {handle_error();}
    result = cyhal_gpio_init( LED2, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    if (result != CY_RSLT_SUCCESS)
    {handle_error();}
    result = cyhal_gpio_init( LED3, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    if (result != CY_RSLT_SUCCESS)
    {handle_error();}

    /*Enable debug output via KitProg UART*/
    result = cy_retarget_io_init( KITPROG_TX, KITPROG_RX, CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
    printf("\x1b[2J\x1b[;H");
    printf("RDK3 RTC Hibernate Example has Started.\r\n");
    printf("The MCU is woken up every %d seconds.\r\n", STAY_OFFLINE_SEC);

    /* Initialize RTC */
    result = cyhal_rtc_init(&rtc_obj);
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    /*Show the system time*/
    result = cyhal_rtc_read(&rtc_obj, &date_time);
    if (CY_RSLT_SUCCESS == result)
    {
        strftime(buffer, sizeof(buffer), "%c", &date_time);
        printf("%s\r\n", buffer);
    }

    for (;;)
    {
    	/*Indicate states with a LED*/
    	cyhal_gpio_write(LED2, CYBSP_LED_STATE_ON);
    	CyDelay(100);
        cyhal_gpio_write(LED2, CYBSP_LED_STATE_OFF);

        /*Should not return from here*/
        CyDelay(STAY_ONLINE_MS);
    	result = Hibernate(&rtc_obj, STAY_OFFLINE_SEC);
        if (CY_RSLT_SUCCESS != result)
        {
        	printf("Failed to enter the system hibernation.\r\n");
            handle_error();
        }
    }
}

/*Hibernate Function*/
cy_rslt_t Hibernate(cyhal_rtc_t *obj, uint32_t seconds)
{
	cy_rslt_t result;
	struct tm date_time;
    time_t UnixTime = {0};

	/*Set the time fields to trigger an alarm event.*/
	cyhal_alarm_active_t alarm_active =
	{
		    alarm_active.en_sec = 1,
		    alarm_active.en_min = 1,
		    alarm_active.en_hour = 1,
		    alarm_active.en_day = 1,
		    alarm_active.en_date = 1,
		    alarm_active.en_month = 1
	};

	result = cyhal_rtc_read(obj, &date_time);
    if (CY_RSLT_SUCCESS != result)
    {return result;}

    /*Get time in Unix format*/
    UnixTime = mktime(&date_time);

    /*Add time in seconds here*/
    UnixTime = UnixTime + seconds;
    /*Convert back*/
    date_time = *(localtime(&UnixTime));

    /*Set the alarm*/
    result = cyhal_rtc_set_alarm(obj, &date_time, alarm_active);
    if (CY_RSLT_SUCCESS != result)
    {return result;}

    /* Enable the alarm event to trigger an interrupt */
    cyhal_rtc_enable_event(obj, CYHAL_RTC_ALARM, RTC_INTERRUPT_PRIORITY, true);

    /* Put the system into a hibernate state. Most of the system will stop. */
    result = cyhal_syspm_hibernate(CYHAL_SYSPM_HIBERNATE_RTC_ALARM);
    if (CY_RSLT_SUCCESS != result)
    {return result;}


    /*Should never reach this*/
    return result;
}

void handle_error(void)
{
     /* Disable all interrupts. */
    __disable_irq();
    cyhal_gpio_write(LED1, CYBSP_LED_STATE_OFF);
    cyhal_gpio_write(LED2, CYBSP_LED_STATE_OFF);
    cyhal_gpio_write(LED3, CYBSP_LED_STATE_ON);
    CY_ASSERT(0);
}

/* [] END OF FILE */
