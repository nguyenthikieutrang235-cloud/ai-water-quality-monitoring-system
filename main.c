/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lcd_i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* Cam bien */
#define TDS_ADC_CHANNEL              ADC_CHANNEL_0
#define TURBIDITY_ADC_CHANNEL        ADC_CHANNEL_8
#define ADC_SAMPLE_COUNT             10
#define FILTER_ALPHA                 0.20f

/* Bien tro dieu chinh toc do mo phong thoi gian
   Chon 1 chan ADC con trong, vi du PA2 = ADC_CHANNEL_2 */
#define POT_ADC_CHANNEL              ADC_CHANNEL_1

/* SRF05 o be chua */
#define WATER_LOW_DISTANCE_CM        25.0f
#define WATER_HIGH_DISTANCE_CM       10.0f

/* Nguong phan loai */
#define TDS_WARNING_PPM_THRESHOLD    500.0f
#define NTU_DIRTY_THRESHOLD          280.0f

/* Bom */
#define PUMP_PORT                    GPIOB
#define PUMP_PIN                     GPIO_PIN_14

/* Neu relay active LOW thi doi 2 dong duoi */
#define PUMP_ON_LEVEL                GPIO_PIN_SET
#define PUMP_OFF_LEVEL               GPIO_PIN_RESET

/* SRF05 */
#define SRF05_TRIG_PORT              GPIOB
#define SRF05_TRIG_PIN               GPIO_PIN_12
#define SRF05_ECHO_PORT              GPIOB
#define SRF05_ECHO_PIN               GPIO_PIN_13

/* LED + BUZZER */
#define LED_GREEN_PORT               GPIOB
#define LED_GREEN_PIN                GPIO_PIN_3
#define LED_YELLOW_PORT              GPIOB
#define LED_YELLOW_PIN               GPIO_PIN_4
#define LED_RED_PORT                 GPIOB
#define LED_RED_PIN                  GPIO_PIN_8
#define BUZZER_PORT                  GPIOB
#define BUZZER_PIN                   GPIO_PIN_9

/* NTU mapping */
#define TURB_VOLT_NTU_0              1.50f
#define TURB_VOLT_NTU_100            1.40f
#define TURB_VOLT_NTU_200            1.30f
#define TURB_VOLT_NTU_300            1.20f
#define TURB_VOLT_NTU_400            1.10f
#define TURB_VOLT_NTU_500            1.00f
#define TURB_VOLT_NTU_700            0.95f
#define TURB_VOLT_NTU_1000           0.90f

/* Mo phong thoi gian
   1 phut ao se tang sau khoang SIM_BASE_TICK_MS / speed_factor ms
   speed_factor lay tu bien tro */
#define SIM_BASE_TICK_MS             1000U

/* 2 moc gio bom */
#define PUMP_TIME1_HOUR              6
#define PUMP_TIME1_MIN               0
#define PUMP_TIME2_HOUR              18
#define PUMP_TIME2_MIN               0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t tds_adc_value = 0;
float tds_voltage = 0.0f;
float tds_ppm = 0.0f;

uint16_t turb_adc_value = 0;
float turb_voltage = 0.0f;
float turb_ntu = 0.0f;

uint16_t pot_adc_value = 0;
float sim_speed_factor = 1.0f;

float distance_cm = -99.0f;
uint32_t pulse_width_us = 0;

uint8_t pump_state = 0;
uint8_t water_quality = 0;      /* 0 clean, 1 warning, 2 dirty */
uint8_t warning_level = 0;      /* 0 green, 1 yellow, 2 red */
uint8_t fill_request = 0;       /* 1 = dang den chu ky bom */

uint8_t display_hold = 0;
uint32_t display_hold_tick = 0;

uint8_t show_due_message = 0;
uint32_t show_due_tick = 0;

char line1[17];
char line2[17];
char last_line1[17] = "";
char last_line2[17] = "";
char uart_tx_buf[220];

const char ready_msg[] = "STM32 WATER SYSTEM READY\r\n";

/* Bien reset LCD khi doi trang thai */
uint8_t last_lcd_state = 0xFF;

/* Dong ho mo phong */
uint8_t sim_hour = 5;
uint8_t sim_min = 58;
uint32_t sim_clock_tick = 0;

/* Danh dau da kich hoat lich bom trong phut hien tai */
uint8_t schedule_triggered_this_minute = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void delay_us(uint16_t us);
void Set_Pump(uint8_t on);
void LCD_PrintLine(uint8_t row, const char *text);
void LCD_Reset(void);
uint8_t LCD_GetStateCode(void);

uint16_t Read_ADC_Channel_Avg(uint32_t channel, uint16_t samples);
float Read_TDS_Voltage(void);
float Read_Turbidity_Voltage(void);
float Convert_TDS_To_PPM(float voltage);
float MapFloat(float x, float in_min, float in_max, float out_min, float out_max);
float Convert_Turbidity_To_NTU(float voltage);
float SRF05_Read_Distance_CM(uint32_t *pulse_us);
float SRF05_Read_Distance_Filtered(uint32_t *pulse_us);

float Read_Sim_Speed_Factor(void);
void Simulated_Clock_Update(void);
void Check_Scheduled_Pump_Request(void);

void Water_Quality_Check(void);
void Pump_Control_With_Quality(float distance);
void Warning_Level_Update(void);
void Alarm_Output_Control(void);
void LCD_Show_Status(void);
void UART_Send_Data(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void delay_us(uint16_t us)
{
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

void Set_Pump(uint8_t on)
{
  HAL_GPIO_WritePin(PUMP_PORT, PUMP_PIN, on ? PUMP_ON_LEVEL : PUMP_OFF_LEVEL);
  pump_state = on ? 1 : 0;
}

void LCD_PrintLine(uint8_t row, const char *text)
{
  char buf[17];
  size_t len = strlen(text);

  if (len > 16) len = 16;

  memset(buf, ' ', 16);
  memcpy(buf, text, len);
  buf[16] = '\0';

  if (row == 0)
  {
    if (strcmp(buf, last_line1) != 0)
    {
      LCD_SetCursor(0, 0);
      LCD_Print(buf);
      strcpy(last_line1, buf);
    }
  }
  else
  {
    if (strcmp(buf, last_line2) != 0)
    {
      LCD_SetCursor(0, 1);
      LCD_Print(buf);
      strcpy(last_line2, buf);
    }
  }
}

void LCD_Reset(void)
{
  LCD_Clear();
  HAL_Delay(2);
  LCD_Init();
  HAL_Delay(5);
  LCD_Backlight();
  HAL_Delay(2);

  last_line1[0] = '\0';
  last_line2[0] = '\0';
}

uint8_t LCD_GetStateCode(void)
{
  uint8_t state = 0;

  state |= (water_quality & 0x03);
  state |= ((warning_level & 0x03) << 2);
  state |= ((fill_request & 0x01) << 4);
  state |= ((pump_state & 0x01) << 5);
  state |= ((display_hold & 0x01) << 6);
  state |= ((show_due_message & 0x01) << 7);

  return state;
}

uint16_t Read_ADC_Channel_Avg(uint32_t channel, uint16_t samples)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  uint32_t sum = 0;
  uint16_t i;

  if (samples == 0) samples = 1;

  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  for (i = 0; i < samples; i++)
  {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 50);
    sum += HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    HAL_Delay(2);
  }

  return (uint16_t)(sum / samples);
}

float Read_TDS_Voltage(void)
{
  static float filtered_voltage = 0.0f;
  float new_voltage;

  tds_adc_value = Read_ADC_Channel_Avg(TDS_ADC_CHANNEL, ADC_SAMPLE_COUNT);
  new_voltage = ((float)tds_adc_value * 3.3f) / 4095.0f;

  if (filtered_voltage == 0.0f)
    filtered_voltage = new_voltage;
  else
    filtered_voltage = (1.0f - FILTER_ALPHA) * filtered_voltage + FILTER_ALPHA * new_voltage;

  return filtered_voltage;
}

float Convert_TDS_To_PPM(float voltage)
{
  float v = voltage;
  float ppm;

  if (v < 0.0f) v = 0.0f;
  if (v > 3.3f) v = 3.3f;

  ppm = (133.42f * v * v * v - 255.86f * v * v + 857.39f * v) * 0.5f;

  if (ppm < 0.0f) ppm = 0.0f;
  if (ppm > 2000.0f) ppm = 2000.0f;

  return ppm;
}

float Read_Turbidity_Voltage(void)
{
  static float filtered_voltage = 0.0f;
  float new_voltage;

  turb_adc_value = Read_ADC_Channel_Avg(TURBIDITY_ADC_CHANNEL, ADC_SAMPLE_COUNT);
  new_voltage = ((float)turb_adc_value * 3.3f) / 4095.0f;

  if (filtered_voltage == 0.0f)
    filtered_voltage = new_voltage;
  else
    filtered_voltage = (1.0f - FILTER_ALPHA) * filtered_voltage + FILTER_ALPHA * new_voltage;

  return filtered_voltage;
}

float MapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float Convert_Turbidity_To_NTU(float voltage)
{
  float ntu;

  ntu = (1.9f - voltage) * 900.0f;

  if (ntu < 0.0f) ntu = 0.0f;
  if (ntu > 1000.0f) ntu = 1000.0f;

  return ntu;
}

float SRF05_Read_Distance_CM(uint32_t *pulse_us)
{
  HAL_GPIO_WritePin(SRF05_TRIG_PORT, SRF05_TRIG_PIN, GPIO_PIN_RESET);
  delay_us(5);

  HAL_GPIO_WritePin(SRF05_TRIG_PORT, SRF05_TRIG_PIN, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(SRF05_TRIG_PORT, SRF05_TRIG_PIN, GPIO_PIN_RESET);

  delay_us(20);

  __HAL_TIM_SET_COUNTER(&htim2, 0);
  while (HAL_GPIO_ReadPin(SRF05_ECHO_PORT, SRF05_ECHO_PIN) == GPIO_PIN_RESET)
  {
    if (__HAL_TIM_GET_COUNTER(&htim2) > 30000)
    {
      *pulse_us = 0;
      return -1.0f;
    }
  }

  __HAL_TIM_SET_COUNTER(&htim2, 0);
  while (HAL_GPIO_ReadPin(SRF05_ECHO_PORT, SRF05_ECHO_PIN) == GPIO_PIN_SET)
  {
    if (__HAL_TIM_GET_COUNTER(&htim2) > 30000)
    {
      *pulse_us = __HAL_TIM_GET_COUNTER(&htim2);
      return -2.0f;
    }
  }

  *pulse_us = __HAL_TIM_GET_COUNTER(&htim2);

  if (*pulse_us < 50)
    return -3.0f;

  return ((float)(*pulse_us) * 0.0343f) / 2.0f;
}

float SRF05_Read_Distance_Filtered(uint32_t *pulse_us)
{
  float d[3];
  uint32_t p;
  float sum = 0.0f;
  int count = 0;
  int i;

  for (i = 0; i < 3; i++)
  {
    d[i] = SRF05_Read_Distance_CM(&p);
    if (d[i] >= 0.0f)
    {
      sum += d[i];
      count++;
      *pulse_us = p;
    }
    HAL_Delay(20);
  }

  if (count == 0)
    return -1.0f;

  return sum / count;
}

float Read_Sim_Speed_Factor(void)
{
  float factor;

  pot_adc_value = Read_ADC_Channel_Avg(POT_ADC_CHANNEL, 8);

  /* map 0..4095 -> 1..60
     nghia la toi da nhanh gap 60 lan
     1 giay that = 60 phut ao */
  factor = 1.0f + ((float)pot_adc_value / 4095.0f) * 59.0f;

  if (factor < 1.0f) factor = 1.0f;
  if (factor > 60.0f) factor = 60.0f;

  return factor;
}

void Simulated_Clock_Update(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t interval_ms;

  sim_speed_factor = Read_Sim_Speed_Factor();

  interval_ms = (uint32_t)((float)SIM_BASE_TICK_MS / sim_speed_factor);
  if (interval_ms < 20U) interval_ms = 20U;

  if ((now - sim_clock_tick) >= interval_ms)
  {
    sim_clock_tick = now;

    sim_min++;
    if (sim_min >= 60)
    {
      sim_min = 0;
      sim_hour++;
      if (sim_hour >= 24)
      {
        sim_hour = 0;
      }
    }
  }
}

void Check_Scheduled_Pump_Request(void)
{
  uint8_t is_schedule_time = 0;

  if ((sim_hour == PUMP_TIME1_HOUR) && (sim_min == PUMP_TIME1_MIN))
    is_schedule_time = 1;

  if ((sim_hour == PUMP_TIME2_HOUR) && (sim_min == PUMP_TIME2_MIN))
    is_schedule_time = 1;

  if (is_schedule_time)
  {
    if (schedule_triggered_this_minute == 0)
    {
      fill_request = 1;
      display_hold = 1;
      display_hold_tick = HAL_GetTick();

      show_due_message = 1;
      show_due_tick = HAL_GetTick();

      schedule_triggered_this_minute = 1;
    }
  }
  else
  {
    schedule_triggered_this_minute = 0;
  }
}

void Water_Quality_Check(void)
{
  if ((tds_ppm <= 300.0f) && (turb_ntu <= 280.0f))
  {
    water_quality = 0;  
  }
  else if ((tds_ppm > 300.0f) || (turb_ntu > 280.0f))
  {
    water_quality = 2; 
  }
  else
  {
    water_quality = 0;  
  }
}

void Pump_Control_With_Quality(float distance)
{
  if (fill_request == 0)
  {
    Set_Pump(0);
    return;
  }

  if (water_quality == 2)
  {
    Set_Pump(0);
    return;
  }

  if (distance < 0.0f)
  {
    Set_Pump(0);
    return;
  }

  if ((distance > WATER_LOW_DISTANCE_CM) && (pump_state == 0))
  {
    Set_Pump(1);
  }
  else if ((distance < WATER_HIGH_DISTANCE_CM) && (pump_state == 1))
  {
    Set_Pump(0);
  }
}

void Warning_Level_Update(void)
{
  if (water_quality == 2)
  {
    warning_level = 2;  
  }
  else
  {
    warning_level = 0;  
  }
}
void Alarm_Output_Control(void)
{
  HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

  if (warning_level == 0)
  {

    HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET);
  }
  else
  {
 
    HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);
  }
}

void LCD_Show_Status(void)
{
  uint8_t current_lcd_state = LCD_GetStateCode();

  if (current_lcd_state != last_lcd_state)
  {
    LCD_Reset();
    last_lcd_state = current_lcd_state;
  }

  /* Hien thong bao den gio bom trong 2 giay */
  if (show_due_message && (HAL_GetTick() - show_due_tick < 2000))
  {
    snprintf(line1, sizeof(line1), "TIME %02u:%02u x%02.0f", sim_hour, sim_min, sim_speed_factor);
    snprintf(line2, sizeof(line2), "DEN GIO BOM");
    LCD_PrintLine(0, line1);
    LCD_PrintLine(1, line2);
    return;
  }
  else
  {
    show_due_message = 0;
  }

  /* Dong 1: thoi gian mo phong */
  snprintf(line1, sizeof(line1), "TIME %02u:%02u ", sim_hour, sim_min);

  /* Dong 2: NTU va PPM cung 1 dong */
  snprintf(line2, sizeof(line2), "NTU%3.0f PPM%4.0f", turb_ntu, tds_ppm);

  LCD_PrintLine(0, line1);
  LCD_PrintLine(1, line2);
}

void UART_Send_Data(void)
{
  if (distance_cm < 0.0f)
  {
    snprintf(uart_tx_buf, sizeof(uart_tx_buf),
             "TIME:%02u:%02u,SPEED:%.1f,PPM:%.0f,NTU:%.1f,D:ERR,Q:%u,F:%u,H:%u,S:%u,P:%u,PULSE:%lu\r\n",
             sim_hour, sim_min, sim_speed_factor, tds_ppm, turb_ntu,
             water_quality, fill_request, display_hold,
             show_due_message, pump_state, pulse_width_us);
  }
  else
  {
    snprintf(uart_tx_buf, sizeof(uart_tx_buf),
             "TIME:%02u:%02u,SPEED:%.1f,PPM:%.0f,NTU:%.1f,D:%.1f,Q:%u,F:%u,H:%u,S:%u,P:%u,PULSE:%lu\r\n",
             sim_hour, sim_min, sim_speed_factor, tds_ppm, turb_ntu,
             distance_cm, water_quality, fill_request, display_hold,
             show_due_message, pump_state, pulse_width_us);
  }

  HAL_UART_Transmit(&huart2, (uint8_t*)uart_tx_buf, strlen(uart_tx_buf), 100);
  HAL_UART_Transmit(&huart1, (uint8_t*)uart_tx_buf, strlen(uart_tx_buf), 100);
}

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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);

  HAL_GPIO_WritePin(SRF05_TRIG_PORT, SRF05_TRIG_PIN, GPIO_PIN_RESET);
  Set_Pump(0);

  HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);

  HAL_Delay(100);
  LCD_Init();
  HAL_Delay(50);
  LCD_Backlight();
  HAL_Delay(20);

  last_line1[0] = '\0';
  last_line2[0] = '\0';

  LCD_PrintLine(0, "HE THONG NUOC");
  LCD_PrintLine(1, "DONG HO AO...");
  HAL_Delay(1200);

  last_line1[0] = '\0';
  last_line2[0] = '\0';
  last_lcd_state = 0xFF;

  sim_clock_tick = HAL_GetTick();

  HAL_UART_Transmit(&huart2, (uint8_t*)ready_msg, strlen(ready_msg), 100);
  HAL_UART_Transmit(&huart1, (uint8_t*)ready_msg, strlen(ready_msg), 100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
 while (1)
{
  Simulated_Clock_Update();
  Check_Scheduled_Pump_Request();

  tds_voltage = Read_TDS_Voltage();
  tds_ppm = Convert_TDS_To_PPM(tds_voltage);

  turb_voltage = Read_Turbidity_Voltage();
  turb_ntu = Convert_Turbidity_To_NTU(turb_voltage);

  Water_Quality_Check();

  if (fill_request)
  {
    distance_cm = SRF05_Read_Distance_Filtered(&pulse_width_us);
  }
  else
  {
    distance_cm = -99.0f;
    pulse_width_us = 0;
  }

  Pump_Control_With_Quality(distance_cm);

  if (display_hold)
  {
    if ((HAL_GetTick() - display_hold_tick) > 2000)
    {
      display_hold = 0;

      if ((distance_cm < 0.0f) || (water_quality == 2) || (pump_state == 0))
      {
        fill_request = 0;
      }
    }
  }

  if (fill_request && (pump_state == 0) &&
      (distance_cm >= WATER_HIGH_DISTANCE_CM) &&
      (distance_cm <= WATER_LOW_DISTANCE_CM))
  {
    fill_request = 0;
    display_hold = 0;
  }

  Warning_Level_Update();
  Alarm_Output_Control();
  LCD_Show_Status();
  UART_Send_Data();

  HAL_Delay(100);
}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
