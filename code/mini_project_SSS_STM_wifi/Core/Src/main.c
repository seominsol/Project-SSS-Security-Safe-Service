/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdio.h>
#include <string.h>
#include "esp.h"
#include "my_lcd_i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PASSWORD_LEN 4

// LCD SCL, SDA GPIO 설정
#define TACT_Pin1 GPIO_PIN_4
#define TACT_GPIO_Port1 GPIOB
#define Ball_SW_PIN GPIO_PIN_5
#define Ball_SW_PORT GPIOB

#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
#define ARR_CNT 5
#define CMD_SIZE 50

#define SQL_TOPIC "SSS_SQL"
#define SAFE_ID   "02"    		 	// 금고 고유 ID
#define TAG_LOGIN "LOGIN"
#define TAG_CHPSW "CHPSWD"        	// 비밀번호 변경
#define TAG_BSWLOCK "SENDLOG"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
//int password[PASSWORD_LEN] = { 0, 0, 0, 0 };
int insert_num[PASSWORD_LEN] = { -1, -1, -1, -1 };

uint32_t last_ball_tick = 0;

volatile uint16_t adc_val;
volatile int      adc_flag = 0;

int flag_locked = 1;		// 금고 잠금 상태 플래그

#define LONG_PRESS_MS 1500   // 원하는 임계값

volatile uint8_t  waiting_release = 0;  // 눌림 이후 릴리즈 대기
volatile uint32_t press_tick = 0;       // 눌린 시각(ms)
volatile uint8_t  short_press_flag = 0; // 짧게 눌림 이벤트
volatile uint8_t  long_press_flag  = 0; // 길게 눌림 이벤트

uint8_t rx2char;
extern cb_data_t cb_data;
extern volatile unsigned char rx2Flag;
extern volatile char rx2Data[50];
volatile int tim3Flag1Sec=1;
volatile unsigned int tim3Sec;

static uint32_t last_adc_tick = 0;
typedef enum { ADC_IDLE, ADC_BUSY } adc_state_t;
static adc_state_t adc_state = ADC_IDLE;

// 버튼 디바운스 처리를 위한 마지막 입력 시간 저장 변수
// 200ms 이내의 중복 입력을 무시하기 위해 사용됨
uint32_t last_press_time_tact1 = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */
static void send_login_to_server(void);
static void send_chpsw_to_server(const int old_pw[4], const int new_pw[4]);
static void code4_to_str(const int d[4], char out[8]);

char strBuff[MAX_ESP_COMMAND_LEN];
void MX_GPIO_LED_ON(int flag);
void MX_GPIO_LED_OFF(int flag);
void esp_event(char *);
char sendBuf[MAX_UART_COMMAND_LEN]={0};
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int CCR = 1500;		// PWM을 통해 서보모터를 제어 할때 모터의 중앙값에 해당하는 값

static void send_ball_event_to_server(void)
{
    if (esp_get_status() != 0) {
        esp_client_conn(); // 연결 안되어 있으면 재연결 시도
        return;
    }

    char msg[64];
    snprintf(msg, sizeof(msg), "[%s]%s@%s\n", SQL_TOPIC, TAG_BSWLOCK, SAFE_ID);
    esp_send_data(msg);
    printf("Ball event sent!\r\n");
}

// 원하는 태그: "PWD_OK", "PWD_NG", "HOLD" 등
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	int ret = 0;
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART2_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("Start main() - wifi\r\n");
  ret |= drv_uart_init();
  ret |= drv_esp_init();
  if(ret != 0)
  {
   printf("Esp response error\r\n");
   Error_Handler();

  }

  AiotClient_Init();
  printf("321321\r\n");
  if(HAL_TIM_Base_Start_IT(&htim3) != HAL_OK)
  {
   Error_Handler();
  }
  printf("321321\r\n");
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);		// PWM 시작


  HAL_TIM_Base_Start(&htim2);					// 타이머 시작
  printf("Start_timer\r\n");
  lcd_init(&hi2c1);							// LCD 초기화
  lcd_set_cursor(0, 3);						// LCD 첫줄 세번 째 칸으로 이동
  lcd_send_string("DIAL LOCK");			    // 문자 출력
  lcd_set_cursor(1, 6);						// LCD 두번째줄 네번 째 칸으로 이동
  lcd_send_string("SAFE");	   		        // 문자 출력
  HAL_Delay(3000);
  lcd_clear();
  printf("initLCD complit\r\n");
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1500);  // 모터 중앙값

  lcd_set_cursor(0, 3);					// LCD 첫줄 세번 째 칸으로 이동
  lcd_send_string("ENTER CODE");			// 문자 출력

  print_nums();

  //HAL_ADC_Start_IT(&hadc1);
  printf("ADC_Start\r\n");
  printf("start while\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  int ball_state = HAL_GPIO_ReadPin(Ball_SW_PORT, Ball_SW_PIN);

	      if (ball_state == GPIO_PIN_RESET) {   // 볼 스위치 감지됨
	          uint32_t now = HAL_GetTick();
	          if (now - last_ball_tick >= 3000) {   // 3초 디바운스
	              last_ball_tick = now;
	              send_ball_event_to_server();      // 서버에 데이터 전송
	          }
	      }

	  uint32_t now = HAL_GetTick();

	  lcd_set_cursor(1, 1);				// LCD 두 번째 줄 두 번째 칸으로 이동
	  lcd_send_string("->");			    // 문자 출력

	      // 300ms 주기
	  	  if (adc_state == ADC_IDLE && (now - last_adc_tick) >= 300) {
	          last_adc_tick = now;
	          HAL_ADC_Start(&hadc1);
	          adc_state = ADC_BUSY;
	      }

	      if (adc_state == ADC_BUSY) {
	          // 블로킹 없이 완료 여부만 체크
	          if (HAL_ADC_PollForConversion(&hadc1, 0) == HAL_OK) {
	              uint16_t val = HAL_ADC_GetValue(&hadc1);

	              int digit;
	              if (val < 200)      digit = 0;
	              else if (val < 500) digit = 1;
	              else if (val < 1000) digit = 2;
	              else if (val < 1500) digit = 3;
	              else if (val < 2000) digit = 4;
	              else if (val < 2500) digit = 5;
	              else if (val < 3000) digit = 6;
	              else if (val < 3500) digit = 7;
	              else if (val < 4000) digit = 8;
	              else                 digit = 9;

	              adc_val = digit;
	              // LCD 출력 (이전 글자 잔상 방지용으로 2글자 덮어쓰기)
	              char buffer[5];
	              lcd_set_cursor(1, 4);
//	              char buf[3] = { '0' + digit, ' ', '\0' };
	              sprintf(buffer, "%d", adc_val);
	              lcd_send_string(buffer);

	              adc_state = ADC_IDLE;       // 다음 주기 대기
	          }
	          // ★ 버튼 눌림 확정 처리: 방금까지 갱신된 adc_val로 입력 확정
//	              if (adc_flag) {
//	                  adc_flag = 0;
//	                  handle_tact1();
//	              }
	          // 아직 완료 안 됐으면 그냥 빠져나가서 Wi-Fi 루프 계속
	      }


	      // 릴리즈 폴링: 눌림 이후 손 뗐는지 확인
	      if (waiting_release) {
	          // 손을 뗐으면 (핀 HIGH)
	          if (HAL_GPIO_ReadPin(TACT_GPIO_Port1, TACT_Pin1) == GPIO_PIN_SET) {
	              waiting_release = 0;
	              uint32_t held = HAL_GetTick() - press_tick;

	              if (held >= LONG_PRESS_MS) long_press_flag = 1;
	              else                       short_press_flag = 1;
	          }
	      }

	      // 플래그 소비(실제 동작 수행)
	      if (long_press_flag) {
	          long_press_flag = 0;
	          // TODO: 길게 누름 동작 (예: reset_nums() 또는 강제 잠금/해제 등)
	      }

	      if (short_press_flag) {
	          short_press_flag = 0;
	          // 짧게 누름: 기존 버튼 한 번 누름과 동일 처리
	          handle_tact1();   // insert_num[i] = adc_val; 등
	      }



	    if(strstr((char *)cb_data.buf,"+IPD") && cb_data.buf[cb_data.length-1] == '\n')
		{
			//?��?��?���??  \r\n+IPD,15:[KSH_LIN]HELLO\n
			strcpy(strBuff,strchr((char *)cb_data.buf,'['));

			//printf("Recv from server: %s\r\n", strBuff);
			memset(cb_data.buf,0x0,sizeof(cb_data.buf));
			cb_data.length = 0;
			esp_event(strBuff);
		}
		if(rx2Flag)
		{
			printf("recv2 : %s\r\n",rx2Data);
			rx2Flag =0;
		}

		if(tim3Flag1Sec)	//1초에 한번
		{
			tim3Flag1Sec = 0;
			if(!(tim3Sec%10)) //10초에 한번
			{
				if(esp_get_status() != 0)
				{
					printf("server connecting ...\r\n");
					esp_client_conn();
				}
			}
//			printf("tim3Sec : %d\r\n",tim3Sec);
//			sprintf(sendBuf,"[%s]%s@%s\n","0","TESTCMD","TESTARG");
//			esp_send_data(sendBuf);


		}
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 83;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 19999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 99;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 38400;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void MX_GPIO_LED_ON(int pin)
{
	HAL_GPIO_WritePin(LD2_GPIO_Port, pin, GPIO_PIN_SET);
}
void MX_GPIO_LED_OFF(int pin)
{
	HAL_GPIO_WritePin(LD2_GPIO_Port, pin, GPIO_PIN_RESET);
}

void esp_event(char * recvBuf)
{
  int i=0;
  char * pToken;
  char * pArray[ARR_CNT]={0};


  strBuff[strlen(recvBuf)-1] = '\0';	//'\n' cut
  printf("\r\nDebug recv : %s\r\n",recvBuf);

  //여기서 문자열 단순 비교
  if (strstr(recvBuf, "SUCCESS")) {

	  // 서보모터 90도 회전
	  CCR = 2500;
	  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, CCR);// PWM  세팅

	  flag_locked = 0;

	  lcd_set_cursor(0, 0);
	  lcd_send_string("                ");
      lcd_set_cursor(0, 4);
      lcd_send_string("SUCCESS!");   // 비밀번호 맞음

  }
  else if (strstr(recvBuf, "FAILURE")) {
	  lcd_set_cursor(0, 0);
	  lcd_send_string("                ");
      lcd_set_cursor(0, 5);
      lcd_send_string("FAILED");    // 비밀번호 틀림
  }

  pToken = strtok(recvBuf,"[@]");
  while(pToken != NULL)
  {
    pArray[i] = pToken;
    if(++i >= ARR_CNT)
      break;
    pToken = strtok(NULL,"[@]");
  }

  if(!strcmp(pArray[1],"LED"))
  {
  	if(!strcmp(pArray[2],"ON"))
  	{
  		MX_GPIO_LED_ON(LD2_Pin);
  	}
	else if(!strcmp(pArray[2],"OFF"))
	{
		MX_GPIO_LED_OFF(LD2_Pin);
	}
	sprintf(sendBuf,"[%s]%s@%s\n",pArray[0],pArray[1],pArray[2]);
  }
  else if(!strncmp(pArray[1]," New conn",8))
  {
//	   printf("Debug : %s, %s\r\n",pArray[0],pArray[1]);
     return;
  }
  else if(!strncmp(pArray[1]," Already log",8))
  {
// 	    printf("Debug : %s, %s\r\n",pArray[0],pArray[1]);
	  esp_client_conn();
      return;
  }
  else
      return;

  esp_send_data(sendBuf);
  printf("Debug send : %s\r\n",sendBuf);
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)		//1ms 마다 호출
{
	static int tim3Cnt = 0;
	tim3Cnt++;
	if(tim3Cnt >= 1000) //1ms * 1000 = 1Sec
	{
		tim3Flag1Sec = 1;
		tim3Sec++;
		tim3Cnt = 0;
	}
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != TACT_Pin1) return;

    uint32_t now = HAL_GetTick();
    if (now - last_press_time_tact1 < 200) return; // 디바운스
    last_press_time_tact1 = now;

    // 풀업/Active-Low 가정: 눌림 시 LOW
    if (HAL_GPIO_ReadPin(TACT_GPIO_Port1, TACT_Pin1) == GPIO_PIN_RESET) {
        press_tick = now;           // 눌린 순간 시각 저장
        waiting_release = 1;        // 릴리즈 대기 시작
        // ※ 여기서 handle_tact1() 호출하지 마세요 (LCD/I2C는 메인에서)
    }
}


//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
//{

//	adc_flag = 1;


//	lcd_set_cursor(1, 1);				// LCD 두 번째 줄 두 번째 칸으로 이동
//	lcd_send_string("->");			    // 문자 출력
//
//	if (hadc->Instance == ADC1)
//	{
//		// ADC 변환 결과를 0~9로 매핑하여 입력 숫자로 설정
//		if (HAL_ADC_GetValue(&hadc1) < 200) 		adc_val = 0;
//		else if (HAL_ADC_GetValue(&hadc1) < 500)	adc_val = 1;
//		else if (HAL_ADC_GetValue(&hadc1) < 1000)	adc_val = 2;
//		else if (HAL_ADC_GetValue(&hadc1) < 1500)	adc_val = 3;
//		else if (HAL_ADC_GetValue(&hadc1) < 2000)	adc_val = 4;
//		else if (HAL_ADC_GetValue(&hadc1) < 2500)	adc_val = 5;
//		else if (HAL_ADC_GetValue(&hadc1) < 3000)	adc_val = 6;
//		else if (HAL_ADC_GetValue(&hadc1) < 3500)	adc_val = 7;
//		else if (HAL_ADC_GetValue(&hadc1) < 4000)	adc_val = 8;
//		else										adc_val = 9;
//
//
//		char buffer[5];
//		lcd_set_cursor(1, 4);
//		sprintf(buffer, "%d", adc_val);
//		lcd_send_string(buffer);
//	}
//	HAL_ADC_Start_IT(&hadc1);
//}

// 4자리 배열(각 원소 0~9 가정)을 "1234" 문자열로 변환
static void code4_to_str(const int d[4], char out[8])
{
    out[0] = '0' + d[0];
    out[1] = '0' + d[1];
    out[2] = '0' + d[2];
    out[3] = '0' + d[3];
    out[4] = '\0';
}

// [SSS_SQL]LOGIN@ID@PSWD\n
static void send_login_to_server(void)
{
    if (esp_get_status() != 0) {
        esp_client_conn();     // 연결 재시도(이번 루프는 그냥 시도만)
        return;
    }

    char code[8];
    code4_to_str(insert_num, code);   // 현재 입력한 4자리

    char msg[80];
    snprintf(msg, sizeof(msg), "[%s]%s@%s@%s\n", SQL_TOPIC, TAG_LOGIN, SAFE_ID, code);
    esp_send_data(msg);
}

// [SSS_SQL]CHPSWD@ID@OLD@NEW\n
static void send_chpsw_to_server(const int old_pw[4], const int new_pw[4])
{
    if (esp_get_status() != 0) {
        esp_client_conn();
        return;
    }

    char oldc[8], newc[8];
    code4_to_str(old_pw, oldc);
    code4_to_str(new_pw, newc);

    char msg[96];
    snprintf(msg, sizeof(msg), "[%s]%s@%s@%s@%s\n", SQL_TOPIC, TAG_CHPSW, SAFE_ID, oldc, newc);
    esp_send_data(msg);
}


void handle_tact1(void)
{
    // 잠금 상태일 때: 숫자 4개 모아서 서버로 전송
    if (flag_locked)
    {
        static int i = 0;

        // 화면 갱신
        if(i == 0){
        lcd_set_cursor(0, 0);
        lcd_send_string("                ");
        lcd_set_cursor(0, 3);
        lcd_send_string("ENTER CODE");
        }

        // 현재 ADC에서 읽은 한 자리 저장
        insert_num[i] = adc_val;
        i++;

        print_nums();

        // 4자리 다 모이면 서버로 전송하고 입력버퍼 초기화
        if (i >= PASSWORD_LEN)
        {
            send_login_to_server();   // [SSS_SQL]LOGIN@ID@PSWD 전송

            // 전송 중/확인 중 안내 (선택)
            lcd_set_cursor(0, 0);
            lcd_send_string("                ");
            lcd_set_cursor(0, 2);
            lcd_send_string("CHECKING....");

            reset_nums();
            i = 0;
        }
    }
    // 잠금 해제 상태일 때: 버튼 누르면 잠금으로 복귀 (기존 동작 유지)
    else
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1500); // 서보 복귀
        lcd_set_cursor(0, 0);
        lcd_send_string("                ");
        lcd_set_cursor(0, 5);
        lcd_send_string("LOCKED");
        flag_locked = 1;
    }
}



void print_nums()
{
	for (int i = 0; i < 4; i++)
	{
		lcd_set_cursor(1, 7 + (i * 2));
		lcd_send_data('0' + insert_num[i]);
	}
}

void reset_nums()
{
	for (int i = 0; i < PASSWORD_LEN; i++)
		insert_num[i] = -1;
}
/*int password_match(int security)
{
		for (int i = 0; i < PASSWORD_LEN; i++)
		{
			if (password[i] != insert_num[i])
				return  0;
		}
		return 1;
}*/
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
