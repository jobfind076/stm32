#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1; // link to esp32s3
UART_HandleTypeDef huart2; // debug output

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

// Retarget printf to USART2 (for GCC / Newlib used by PlatformIO)
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

// Blocks until '\n' is received or buffer is full, then null-terminates the string
void UART_ReadString(UART_HandleTypeDef *huart, char *buffer, uint16_t max_len)
{
    uint16_t idx = 0;
    uint8_t ch = 0;

    while (idx < (max_len - 1))
    {
        // Wait indefinitely (HAL_MAX_DELAY) for 1 byte
        if (HAL_UART_Receive(huart, &ch, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if (ch == '\r')
            {
                continue; // Ignore carriage return
            }
            if (ch == '\n')
            {
                break; // End of string reached
            }
            buffer[idx++] = (char)ch;
        }
    }
    buffer[idx] = '\0'; // Null-terminate C-string
}

// USART1 Init: PA9 (TX), PA10 (RX)
static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

// USART2 Init: PA2 (TX), PA3 (RX)
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

// Low-level hardware GPIO & Clock initialization called automatically by HAL_UART_Init
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // PA9 -> USART1_TX, PA10 -> USART1_RX
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // PA2 -> USART2_TX, PA3 -> USART2_RX
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

// Required by STM32Cube HAL for HAL_Delay() and timeouts
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/// Basic HSI 16MHz Clock Config
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    char rx_buffer[100];
    int len;

    printf("hello from stm32\n");

    while (1)
    {
        rx_buffer[0] = 0;
        // Execution pauses here until a full line is received
        UART_ReadString(&huart1, rx_buffer, sizeof(rx_buffer));

        len = strlen(rx_buffer);
        if (len != 0)
        {
            printf("received %d bytes: %s\r\n", len, rx_buffer);
        }
    }
}
