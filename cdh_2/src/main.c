#include "global.h"
#include "util.h"
#include "stm32f4/core.h"
#include "usb_controller.h"
#include "diskio.h"
#include "ff.h"
#include "stm32f4/radio.h"
#include "stm32f4/adc.h"
#include "stm32f4/camera.h"

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

volatile uint64_t time_ms = 0;

// ##########################
// ##### FREERTOS HOOKS #####
// ##########################

// Hook to increment the system timer
void vApplicationTickHook(void){
    time_ms++;
    if(!(time_ms % 10))
      disk_timerproc();
}

// Hook to execute whenever entering the idle task
// This is a good place for power-saving code.
void vApplicationIdleHook(void) { } // __WFI(); }

// Hook to handle stack overflow
void vApplicationStackOverflowHook(xTaskHandle xTask,
                                   signed portCHAR *pcTaskName){}

// ################
// ##### MAIN #####
// ################

#pragma location = "SRAMSection" // Place the next variable in SRAM
uint8_t mem[128]; // This is placed in SRAM
uint8_t k; // This is placed in normal memory because it isn't preceded with the #pragma location directive

void rs232_task(void* pv) {
  char *data = "hello\0";
  while(1)
  {    
    Radio_SendData(data, strlen(data));
    vTaskDelay(100);
  }
}

void adc_task(void* pv) {
  float reading = 0;
  while(1)
  {    
    reading = adc_GetChannelVoltage(ADC_SNS_3V0);
    vTaskDelay(100);
  }
}


//#######
//CAMERA
//#######
void u8_task(void *pv) {

  fprintf(stdout, "Begin u8_task.\n");
  
  
  vTaskSuspend(NULL);
}




//#######
//CAMERA
//#######
void camera_task(void *pv) {

   fprintf(stdout, "Begin camera_task.\n");
  
  vTaskDelay(100);
  
  fprintf(stderr, "CAMERA TASK: Enabling camera\n");
  Camera_Cmd(ENABLE);
  fprintf(stderr, "CAMERA TASK: Beginning recording\n");
  Camera_Record_Snippet(10000);
  vTaskDelay(10000);
  fprintf(stderr, "CAMERA TASK: Disabling camera\n");
  Camera_Cmd(DISABLE);

  
  vTaskDelay(100);
  
  vTaskDelay(100);
  
  fprintf(stdout, "Begin FS write.\n");
  
  //{
    FATFS fs;
    FIL file;
    
    if(f_mount(0, &fs) != FR_OK){
      fprintf(stderr, "ERROR: could noit mount file system.\n");
      while(1) ;
    }
    
    if(f_open(&file, "temp1.txt", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK){
      fprintf(stderr, "ERROR: could not open temp1.txt.\n");
      while(1) ;
    }
    
    uint32_t bytes_written = 0;
    f_write(&file, "test\r\n", 6, &bytes_written);
    
    f_sync(&file);
    f_close(&file);
    
    f_mount(0, NULL);
    
    uint8_t cmd = 0;
    disk_ioctl(0, CTRL_POWER, &cmd);
    
    if(f_mount(0, &fs) != FR_OK)
      while(1) ;
    
    if(f_open(&file, "temp2.txt", FA_WRITE | FA_OPEN_ALWAYS) != FR_OK)
      while(1) ;
    
    bytes_written = 0;
    f_write(&file, "test\r\n", 6, &bytes_written);
    
    f_sync(&file);
    f_close(&file);
    
    f_mount(0, NULL);
        
    
  //}
  
  
  vTaskDelay(100);
  
  fprintf(stdout, "End FS write.\n");
  
  
  /*
  //NEVER REACHED
  
  vTaskDelay(100);
  
  fprintf(stderr, "Enabling camera\n");
  Camera_Cmd(ENABLE);
  Camera_Record_Snippet(4000);
  vTaskDelay(1000);
  Camera_Cmd(DISABLE);
  fprintf(stderr, "Disabling camera\n");
  
  vTaskDelay(100);*/
  
  vTaskSuspend(NULL);
}

//########
//END CAMERA
//########




void sram_check_task(void* pv) {
  /* hack initialization code for now */
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
  
  // yellow LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  // green LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  // blue LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOF, &GPIO_InitStructure);
  
  // clear SRAM
  memset((void*)0x60000000, 0x00, 0x400000);
  
  // all on then off
  GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
  GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_SET);
  GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_SET);
  vTaskDelay(1000);
  GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
  GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_RESET);
  GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
  
  
  uint32_t i, j;
  uint8_t problem = 0;
  for(i = 0x60000000; i < 0x60400000; i++) {
    // set all of memory to blank
    memset((void*)0x60000000, 0x00, 0x400000);
    
    // set a byte at the current address to be zero
    memset((void *) i, 0xFF, 0x01);
    
    // check to make sure that no other bytes are 0xFF in the memory space
    for(j = 0x60000000; j < 0x60400000; j++) {
      // ignore the location we know to be written
      if(i == j) continue;
      
      // note that there was a problem if we see anything non-zero
      if(*((uint32_t *) j) == 0xFF) {
        problem = 1; 
        break;
      }
    }
    if(problem != 0) {
      // light up the yellow LED to indicate a problem
      GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
      while(1) vTaskDelay(1000);
    }
    // light up the blue LED for 10ms to show a successful location.
    GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_SET);
    vTaskDelay(10);
    GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
  }
  // light up the green LED and stall to show success
  GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_SET);
  
  while(1) vTaskDelay(1000);
  
}

void sram_check_brief_task(void* pv) {
  /* hack initialization code for now */
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit(&GPIO_InitStructure);
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
  
  // yellow LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  // green LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  // blue LED
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_Init(GPIOF, &GPIO_InitStructure);  
  
  // clear SRAM
  memset((void*)0x60000000, 0x00, 0x400000);
  
  // all on then off
  GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
  GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_SET);
  GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_SET);
  vTaskDelay(1000);
  GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
  GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_RESET);
  GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
  
  
  uint32_t i, j;
  uint8_t problem = 0;
  uint32_t offset = 0x60000000;
  // unlike the full check, this writes the first byte at each power of 2 (MUCH faster)
  for(i = 1; i < 0x400000; i = i << 1) {
    // set all of memory to blank
    memset((void*)0x60000000, 0x00, 0x400000);
    
    // set a byte at the current address to be zero
    memset((void *) (i + offset), 0xFF, 0x01);
    
    // check to make sure that no other bytes are 0xFF in the memory space
    for(j = 0x60000000; j < 0x60400000; j++) {
      // ignore the location we know to be written
      if(i + offset == j) continue;
      
      // note that there was a problem if we see anything non-zero
      if(*((uint32_t *) j) == 0xFF) {
        problem = 1; 
        break;
      }
    }
    if(problem != 0) {
      // light up the yellow LED to indicate a problem
      GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
      while(1) vTaskDelay(1000);
    }
    // light up the blue LED for 10ms to show a successful location.
    GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_SET);
    vTaskDelay(10);
    GPIO_WriteBit(GPIOF, GPIO_Pin_10, Bit_RESET);
  }
  // light up the green LED and stall to show success
  GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_SET);
  
  while(1) vTaskDelay(1000);
  
}

int main()
{
  // Start the CPU cycle counter (read with CYCCNT)
  DWT_CTRL |= DWT_CTRL_CYCEN;
  
  // Populate unique id
  populateUniqueID();
  
  // Initialize modules
  Core_Init();
  // USB_Init();
  
  fprintf(stdout, "Begin Main at %u.\n", time_ms);
  
  //xTaskCreate( task, ( signed char * ) "task", configMINIMAL_STACK_SIZE + 200, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );
  //xTaskCreate( rs232_task, ( signed char * ) "rs232 test", configMINIMAL_STACK_SIZE + 200, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );
  //xTaskCreate( adc_task, ( signed char * ) "adc test", configMINIMAL_STACK_SIZE + 200, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );
  xTaskCreate( camera_task, ( signed char * ) "camera test", 10000, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );
  
  //xTaskCreate( u8_task, ( signed char * ) "u8 status test", 10000, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );

  //xTaskCreate(sram_check_task,  ( signed char * ) "sram test", 4000, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );
  //xTaskCreate(sram_check_brief_task,  ( signed char * ) "sram test (fast)", 4000, ( void * ) NULL, tskIDLE_PRIORITY + 1, NULL );
  
  // Start the scheduler
  vTaskStartScheduler();

  // Should never get here
  while(1);
}
