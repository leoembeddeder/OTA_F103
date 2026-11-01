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
#include "can.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "can_frame.h"
#include "can_ring_buffer.h"
#include "isotp_fd.h"
#include "uds_types.h"
#include "uds_server.h"
#include "uds_session.h"
#include "dtc_table_85.h"
#include "dtc_store.h"
#include "dtc_nvm_adapter.h"
#include "uds_wear_leveling.h"
#include "uds_platform_time.h"



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

/* USER CODE BEGIN PV */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

can_ring_buffer_t queue;
/* Push recieve CAN frames */
can_frame_t rx_frame;
can_frame_t decode;

static isotp_fd_channel_t s_phys;

static bool isotp_tx_cb(const can_frame_t *f, void *user);


/* ── function definition ────────────────── */


bool can_hw_send(const can_frame_t *frame) 
{ 
    if (CAN_TX(frame->id, (uint8_t*)frame->data, frame->dlc) != HAL_OK)
    {
        return false;
    }
	return true; 
}


static bool isotp_tx_cb(const can_frame_t *f, void *user) {
    (void)user;
    return can_hw_send(f);
}


/* ── App config (registered with uds_server_init) ────────────────── */

static const uds_app_config_t s_app_cfg = {

};

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
  MX_CAN_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  CAN_Handler_Init();
  
  can_ring_buffer_init(&queue);
  isotp_fd_init(&s_phys, ISOTP_RX_ID, ISOTP_TX_ID, false,isotp_tx_cb, NULL);
  uds_server_init(&s_app_cfg);

  printf("stm32 power on reset!\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	//Parsing CAN data frames in a ringbuffer queue
	while(can_ring_buffer_pop(&queue, &decode))
	{
		 if ((decode.id == ISOTP_RX_ID) ||
		 	((decode.id == ISOTP_FUNC_RX_ID)&& ((decode.data[0] & 0xF0u) == 0u)))
		 {
			isotp_fd_on_rx(&s_phys, &decode, HAL_GetTick());
		 }
	}
	isotp_fd_poll(&s_phys, HAL_GetTick());
	
	if (isotp_fd_rx_ready(&s_phys)) 
	{
		uint32_t req_len;
		const uint8_t *req = isotp_fd_rx_data(&s_phys, &req_len);
		uds_response_t resp;
		bool send = uds_server_process(req, req_len, false, &resp);
		isotp_fd_rx_done(&s_phys);
	
		if (send) 
		{
			isotp_fd_send(&s_phys, resp.data, resp.len, HAL_GetTick());
		}
	}
	uds_server_poll();

	
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

/* USER CODE BEGIN 4 */
/* Receive FIFO 0 message pending interrupt management */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef hdr;
	memset(&rx_frame, 0, sizeof(rx_frame));
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, rx_frame.data) == HAL_OK)
    {
    	rx_frame.id    = hdr.StdId;
		rx_frame.dlc   = hdr.DLC;
		rx_frame.is_ext = false;
		rx_frame.is_rtr = false;
		rx_frame.is_fd  = false;

		if (rx_frame.id == ISOTP_RX_ID || rx_frame.id == ISOTP_FUNC_RX_ID) 
		{
			/* Push recieve CAN frames */
			(void)can_ring_buffer_push(&queue, &rx_frame);
		}
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
