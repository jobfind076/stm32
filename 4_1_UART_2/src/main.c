#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1; // link to esp32s3
UART_HandleTypeDef huart2; // debug output
uint8_t rx_byte = 0;

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
uint8_t UART_ReadCode(UART_HandleTypeDef *huart, char *buffer, uint16_t max_len)
{
    uint8_t found = 0;

    // Wait indefinitely (HAL_MAX_DELAY) for 1 byte
    if (HAL_UART_Receive(huart, (uint8_t *)buffer, 1, HAL_MAX_DELAY) == HAL_OK)
    {
        found = 1;
    }

    return found;
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
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // Enable USART1 Interrupt in NVIC
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // PA2 -> USART2_TX, PA3 -> USART2_RX
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure PA0 as Input
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure PA1 as Push-Pull Output (LED)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
}

// Interrupt handler required for HAL UART IT to work
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
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

// Callback triggered automatically when 1 byte is received on UART1
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rx_byte == 1)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); // Turn LED ON
            printf("led turned on\r\n");
        }
        else if (rx_byte == 0)
        {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // Optional: '0' turns LED OFF
            printf("led turned off\r\n");
        }

        // Re-enable interrupt for the next byte
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    printf("hello from stm32\r\n");

    // Start listening for incoming UART bytes via interrupt
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    uint8_t last_button_state = GPIO_PIN_RESET;

    while (1)
    {
        uint8_t code = 1;
        uint8_t current_button_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

        // Detect rising edge (button press) with basic debounce
        if (current_button_state == GPIO_PIN_SET && last_button_state == GPIO_PIN_RESET)
        {
            HAL_Delay(50); // Debounce delay

            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
            {
                code = 0;
            }

            HAL_UART_Transmit(&huart1, &code, 1, 100);
            printf("sent 0\r\n");
        }

        if (current_button_state == GPIO_PIN_RESET && last_button_state == GPIO_PIN_SET)
        {
            HAL_Delay(50); // Debounce delay

            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
            {
                code = 1;
            }

            HAL_UART_Transmit(&huart1, &code, 1, 100);
            printf("sent 1\r\n");
        }

        last_button_state = current_button_state;
        HAL_Delay(10);
    }
}
