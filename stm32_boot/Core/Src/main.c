/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint32_t update_flag;
    uint32_t firmware_size;
    uint32_t firmware_crc;
} ota_config_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ACTIVE_APP_ADDR         0x08002000
#define DOWNLOAD_BANK_ADDR      0x08009000
#define OTA_CONFIG_ADDR         0x0800FC00
#define FLASH_PAGE_SIZE         1024
#define APP_MAX_SIZE            (28 * 1024)

#define OTA_FLAG_PENDING        0xAA55AA55
#define OTA_FLAG_SUCCESS        0x55AA55AA
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
/* USER CODE BEGIN PFP */
void jump_to_app(void);
void copy_download_to_active(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  ota_config_t *config = (ota_config_t *)OTA_CONFIG_ADDR;

  if (config->update_flag == OTA_FLAG_PENDING) {
      // 1. Copy firmware from Download Bank to Active App Bank
      copy_download_to_active();
      
      // 2. Erase OTA Config sector and write SUCCESS flag
      HAL_FLASH_Unlock();
      FLASH_EraseInitTypeDef EraseInitStruct;
      uint32_t PageError = 0;
      EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
      EraseInitStruct.PageAddress = OTA_CONFIG_ADDR;
      EraseInitStruct.NbPages     = 1;
      
      if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) == HAL_OK) {
          HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, OTA_CONFIG_ADDR, OTA_FLAG_SUCCESS);
      }
      HAL_FLASH_Lock();
      
      // 3. Trigger a system reset to start fresh and boot the new app
      NVIC_SystemReset();
  }

  // 4. Jump to the Active Application
  jump_to_app();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void copy_download_to_active(void)
{
    ota_config_t *config = (ota_config_t *)OTA_CONFIG_ADDR;
    uint32_t size = config->firmware_size;
    
    // Clamp size to maximum active app size (28 KB)
    if (size > APP_MAX_SIZE || size == 0) {
        size = APP_MAX_SIZE;
    }
    
    HAL_FLASH_Unlock();

    // 1. Erase Active App Bank page by page
    for (uint32_t addr = ACTIVE_APP_ADDR; addr < ACTIVE_APP_ADDR + size; addr += FLASH_PAGE_SIZE) {
        FLASH_EraseInitTypeDef EraseInitStruct;
        uint32_t PageError = 0;
        EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
        EraseInitStruct.PageAddress = addr;
        EraseInitStruct.NbPages     = 1;
        HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);
    }

    // 2. Copy data from Download Bank to Active App Bank word-by-word
    uint32_t *src = (uint32_t *)DOWNLOAD_BANK_ADDR;
    uint32_t *dst = (uint32_t *)ACTIVE_APP_ADDR;
    uint32_t words_to_copy = (size + 3) / 4;
    
    for (uint32_t i = 0; i < words_to_copy; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)(dst + i), src[i]);
    }

    HAL_FLASH_Lock();
}

void jump_to_app(void)
{
    uint32_t app_jump_address;
    void (*app_reset_handler)(void);

    uint32_t app_stack_ptr = *(volatile uint32_t*)ACTIVE_APP_ADDR;
    
    // Validate if stack pointer points to RAM (0x20000000)
    if ((app_stack_ptr & 0x2FFE0000) == 0x20000000) {
        app_jump_address = *(volatile uint32_t*)(ACTIVE_APP_ADDR + 4);
        app_reset_handler = (void (*)(void))app_jump_address;

        // Reset peripherals and clocks
        HAL_RCC_DeInit();
        HAL_DeInit();
        
        // Disable SysTick
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;

        // Relocate vector table offset
        SCB->VTOR = ACTIVE_APP_ADDR;

        // Set MSP and jump
        __set_MSP(app_stack_ptr);
        app_reset_handler();
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
