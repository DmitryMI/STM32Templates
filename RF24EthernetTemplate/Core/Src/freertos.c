/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */

	size_t freeHeap = xPortGetFreeHeapSize();
	printf("Malloc failed. Free heap size was %u\n", freeHeap);

	HeapStats_t xHeapStats;
	vPortGetHeapStats(&xHeapStats);

	printf("[Heap Stats] xAvailableHeapSpaceInBytes: %u\n", xHeapStats.xAvailableHeapSpaceInBytes);
	printf("[Heap Stats] xSizeOfLargestFreeBlockInBytes: %u\n", xHeapStats.xSizeOfLargestFreeBlockInBytes);
	printf("[Heap Stats] xSizeOfSmallestFreeBlockInBytes: %u\n", xHeapStats.xSizeOfSmallestFreeBlockInBytes);
	printf("[Heap Stats] xNumberOfFreeBlocks: %u\n", xHeapStats.xNumberOfFreeBlocks);
	printf("[Heap Stats] xMinimumEverFreeBytesRemaining: %u\n", xHeapStats.xMinimumEverFreeBytesRemaining);
	printf("[Heap Stats] xNumberOfSuccessfulAllocations: %u\n", xHeapStats.xNumberOfSuccessfulAllocations);
	printf("[Heap Stats] xNumberOfSuccessfulFrees: %u\n", xHeapStats.xNumberOfSuccessfulFrees);

	__disable_irq();
	while(1)
	{

	}
}
/* USER CODE END 5 */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

