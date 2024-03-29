/**
* \file  main.c
*
* \brief LORAWAN Demo Application main file
*		
*
* Copyright (c) 2018 Microchip Technology Inc. and its subsidiaries. 
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products. 
* It is your responsibility to comply with third party license terms applicable 
* to your use of third party software (including open source software) that 
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, 
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, 
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE 
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE 
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY 
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, 
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/
 
/****************************** INCLUDES **************************************/
#include "asf.h"
#include "system_low_power.h"
#include "radio_driver_hal.h"
#include "lorawan.h"
#include "sys.h"
#include "system_init.h"
#include "system_assert.h"
#include "aes_engine.h"
#include "enddevice_demo.h"
#include "sio2host.h"
#include "extint.h"
#include "conf_app.h"
#include "sw_timer.h"
#include "adc.h"
#ifdef CONF_PMM_ENABLE
#include "pmm.h"
#include  "conf_pmm.h"
#include "sleep_timer.h"
#include "sleep.h"
#endif
#include "conf_sio2host.h"
#if (ENABLE_PDS == 1)
#include "pds_interface.h"
#endif
#if (CERT_APP == 1)
#include "conf_certification.h"
#include "enddevice_cert.h"
#endif
#include "sal.h"
/************************** Macro definition ***********************************/
/* Button debounce time in ms */
#define APP_DEBOUNCE_TIME       50
/************************** Global variables ***********************************/
//float acc_val=0;
//static char acc_sen_str[25];

bool button_pressed = false;
bool factory_reset = false;
bool bandSelected = false;
uint32_t longPress = 0;
uint8_t demoTimerId = 0xFF;
uint8_t lTimerId = 0xFF;
extern bool certAppEnabled;
#ifdef CONF_PMM_ENABLE
bool deviceResetsForWakeup = false;
#endif

struct adc_module adc_instance;

//struct adc_config conf_adc;
/************************** Extern variables ***********************************/

/************************** Function Prototypes ********************************/
static void driver_init(void);
//static uint16_t adc_start_read_result(void);

static void ADC_start(void);
//static void get_adc_resource_data(uint8_t * data);
//static void get_adc_data(uint8_t *data);
//static double acc_sensor_value(int type);
//static float calculate_acc(uint16_t raw_code);



#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code);
#endif /* #if (_DEBUG_ == 1) */


/****************************** FUNCTIONS **************************************/

static void print_reset_causes(void)
{
    enum system_reset_cause rcause = system_get_reset_cause();
    printf("Last reset cause: ");
    if(rcause & (1 << 6)) {
        printf("System Reset Request\r\n");
    }
    if(rcause & (1 << 5)) {
        printf("Watchdog Reset\r\n");
    }
    if(rcause & (1 << 4)) {
        printf("External Reset\r\n");
    }
    if(rcause & (1 << 2)) {
        printf("Brown Out 33 Detector Reset\r\n");
    }
    if(rcause & (1 << 1)) {
        printf("Brown Out 12 Detector Reset\r\n");
    }
    if(rcause & (1 << 0)) {
        printf("Power-On Reset\r\n");
    }
}


#if (_DEBUG_ == 1)
static void assertHandler(SystemAssertLevel_t level, uint16_t code)
{
    printf("\r\n%04x\r\n", code);
    (void)level;
}
#endif /* #if (_DEBUG_ == 1) */

/**
 * \mainpage
 * \section preface Preface
 * This is the reference manual for the LORAWAN Demo Application of EU Band
 */
int main(void)
{
	
    /* System Initialization */
    system_init();
    /* Initialize the delay driver */
    delay_init();
    /* Initialize the board target resources */
    board_init();

    INTERRUPT_GlobalInterruptEnable();
	/* Initialize the Serial Interface */
	sio2host_init();


    /* Initialize Hardware and Software Modules */
	driver_init();
   
    delay_ms(5);
    print_reset_causes();
#if (_DEBUG_ == 1)
    SYSTEM_AssertSubscribe(assertHandler);
#endif
    /* Initialize demo application */
    Stack_Init();

    SwTimerCreate(&demoTimerId);
    SwTimerCreate(&lTimerId);

	ADC_start();
    
	mote_demo_init();
	
	
	//adc_flush(&adc_instance);
	
    while (1)
    {
        SYSTEM_RunTasks();
    }
}

static void ADC_start(void)
{
	struct adc_config conf_adc;
	
	adc_get_config_defaults(&conf_adc);
	
	conf_adc.clock_source = GCLK_GENERATOR_2;
	conf_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV2;
	conf_adc.reference = ADC_REFCTRL_REFSEL_INTVCC0;
	conf_adc.positive_input =  ADC_POSITIVE_INPUT_PIN6;
	conf_adc.negative_input = ADC_NEGATIVE_INPUT_GND;
	conf_adc.sample_length = 63;
	
	adc_init(&adc_instance, ADC, &conf_adc);

	adc_enable(&adc_instance);
}


/* Initializes all the hardware and software modules used for Stack operation */
static void driver_init(void)
{
	SalStatus_t sal_status = SAL_SUCCESS;
    /* Initialize the Radio Hardware */
    HAL_RadioInit();
    /* Initialize the Software Timer Module */
    SystemTimerInit();
#ifdef CONF_PMM_ENABLE
    /* Initialize the Sleep Timer Module */
    SleepTimerInit();
#endif
#if (ENABLE_PDS == 1)
    /* PDS Module Init */
    PDS_Init();
#endif
	/* Initializes the Security modules */
	sal_status = SAL_Init();
	
	if (SAL_SUCCESS != sal_status)
	{
		printf("Initialization of Security module is failed\r\n");
		/* Stop Further execution */
		while (1) {
		}
	}
}


