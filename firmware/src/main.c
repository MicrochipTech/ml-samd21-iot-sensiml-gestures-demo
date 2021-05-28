/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include "buffer.h"
#include "sensor.h"
#include "app_config.h"
#if SENSIML_BUILD
#include "kb.h"
#endif

#define SYSTICK_FREQ_IN_MHZ 48 // !NB! This must be changed if the processor clock changes

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

static volatile uint32_t tickcounter = 0;
static uint8_t led_mask = 0;

static struct sensor_device_t sensor;
static struct sensor_buffer_t snsr_buffer;

void led_toggle(uint8_t led_mask) {
    if (led_mask & 1) LED_BLUE_Toggle();
    if (led_mask & 2) LED_GREEN_Toggle();
    if (led_mask & 4) LED_YELLOW_Toggle();
    if (led_mask & 8) LED_RED_Toggle();
}

void SYSTICK_Callback(uintptr_t context) {
    static int mstick = 0;
    int tickrate = *((int *) context);
    
    ++tickcounter;
    if (tickrate == 0) {
        mstick = 0;
    }
    else if (++mstick == tickrate) {
        led_toggle(led_mask);
        mstick = 0;
    }
}

uint64_t read_timer_ms(void) {
    return tickcounter;
}

uint64_t read_timer_us(void) {
    return tickcounter * 1000 + SYSTICK_TimerCounterGet() / SYSTICK_FREQ_IN_MHZ;
}

void sleep_ms(uint32_t ms) {
    SYSTICK_TimerStop();
    tickcounter = 0;
    SYSTICK_TimerStart();
    while (read_timer_ms() < ms) {};
}

void sleep_us(uint32_t us) {
    SYSTICK_TimerStop();
    tickcounter = 0;
    SYSTICK_TimerStart();
    while (read_timer_us() < us) {};
}


// For handling read of the sensor data
void SNSR_ISR_HANDLER(uintptr_t context) {
    struct sensor_device_t *sensor = (struct sensor_device_t *) context;
    
    /* Check if any errors we've flagged have been acknowledged */
    if (sensor->status != SNSR_STATUS_OK) {
        return;
    }
    
    sensor->status = sensor_read(sensor, &snsr_buffer);
}

/* Original mapping */
//1 - figeight
//2 - idle
//3 - unknown
//4 - updown
//5 - wave
//6 - wheel

/* Squashed down mapping for LEDs */
//1 - figeight
//2 - updown
//3 - wave
//4 - wheel

const char class_map[7][16] = {
    "UNK",
    "figure eight",
    "idle",
    "unknown",
    "up-down",
    "wave",
    "wheel"
};

int main ( void )
{
    int tickrate = 0;
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    /* Register and start the LED ticker */
    SYSTICK_TimerCallbackSet(SYSTICK_Callback, (uintptr_t) &tickrate);
    SYSTICK_TimerStart();
    
    /* Activate External Interrupt Controller for sensor capture */
    EIC_CallbackRegister(MIKRO_EIC_PIN, SNSR_ISR_HANDLER, (uintptr_t) &sensor);
    EIC_InterruptEnable(MIKRO_EIC_PIN);
       
    /* Initialize our data buffer */
    buffer_init(&snsr_buffer);
    
    printf("\r\n");
    
    while (1)
    {    
        if (sensor_init(&sensor) != SNSR_STATUS_OK) {
            printf("sensor init result = %d\r\n", sensor.status);
            break;
        }
        
        if (sensor_set_config(&sensor) != SNSR_STATUS_OK) {
            printf("sensor configuration result = %d\r\n", sensor.status);
            break;
        }
        
        printf("sensor sample rate set at %dHz\r\n", SNSR_SAMPLE_RATE);
        
#if SENSIML_BUILD
        /* Initialize SensiML Knowledge Pack */
        kb_model_init();
        
        const uint8_t *ptr = kb_get_model_uuid_ptr(0);
        printf("Running SensiML knowledge pack uuid ");
        for (int i=0; i < 16; i++) { printf("%02x", *ptr++); }
        printf("\r\n");
#endif
        buffer_reset(&snsr_buffer);
        break;
    }
    
    int clsid = 0;
    uint32_t runtime = 0;
    while (1)
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        if (sensor.status != SNSR_STATUS_OK) {
            printf("Got a bad sensor status: %d\r\n", sensor.status);
            break;
        }
        else if (snsr_buffer.overrun == true) {
            printf("\r\n\r\n\r\nOverrun!\r\n\r\n\r\n");
            
            // Light the LEDs to indicate overflow
            tickrate = 0;
            LED_ALL_Off();
            LED_YELLOW_On();  // Indicate OVERFLOW
            sleep_ms(5000U);
            LED_YELLOW_Off(); // Clear OVERFLOW
            
            buffer_reset(&snsr_buffer);
#if SENSIML_BUILD
            kb_flush_model_buffer(0);
#endif            
            continue;
        }     
        else {
            // Feed temp buffer
            buffer_data_t *ptr;
            int rdcnt = buffer_get_read_buffer(&snsr_buffer, &ptr);

            while ( --rdcnt >= 0 ) {
#if DATA_VISUALIZER_BUILD
                uint8_t headerbyte = MPDV_START_OF_FRAME;
                
                SERCOM5_USART_Write(&headerbyte, 1);
                
                SERCOM5_USART_Write(&temp_buffer[tempIdx][0], SNSR_NUM_AXES*sizeof(buffer_data_t));
                
                headerbyte = ~headerbyte;
                SERCOM5_USART_Write(&headerbyte, 1);
                headerbyte = ~headerbyte;
#elif DATA_LOGGER_BUILD
                printf("%d", temp_buffer[tempIdx][0]);
                for (int j=1; j < SNSR_NUM_AXES; j++) {
                    printf(" %d", temp_buffer[tempIdx][j]);
                }
                printf("\r\n");
#elif SENSIML_BUILD
                runtime = tickcounter;
                int ret = kb_run_model(ptr, SNSR_NUM_AXES, 0);
                if (ret >= 0) {
                    runtime = tickcounter - runtime;
                    clsid = ret;
                    
                    tickrate = 0;
                    LED_ALL_Off();
                    if (clsid == 0 || clsid == 3) {
                        ;
                    }
                    else if (clsid == 2) {
                        LED_YELLOW_On();
                    }
                    else { // if ((lastclsid == 0) || (lastclsid == 2) || (clsid == lastclsid)) {
                        // Squash down the class ID, skipping 'idle' and 'unknown' state
                        led_mask = (uint8_t) (1 << (((clsid == 1) ? 1 : (clsid - 2)) - 1));
                        tickrate = TICK_RATE_FAST;
                    }
                    
                    kb_reset_model(0);
                    printf("Gesture classified as '%s' in %lums\r\n", class_map[clsid], runtime);
                }
#endif
                buffer_advance_read_index(&snsr_buffer, 1);
                ptr += SNSR_NUM_AXES;
            }
        }
        
    }
    
    tickrate = 0;
    LED_GREEN_Off();
    LED_RED_On();
    
    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

