/***************************************************************************//**
 * @file
 * @brief init_app.c
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#include "hal-config.h"
#include "hal-config-board.h"
#else
#include "bspconfig.h"
#endif

#include "bsp.h"
#include "i2cspm.h"
#include "pti.h"
#include "gpiointerrupt.h"

#define EXTI_BUTTON0 6
#define EXTI_BUTTON1 7

extern void btn0_irq_hook(void);
extern void btn1_irq_hook(void);
static void gpioInterruptHandler(uint8_t pin)
{
  if (pin == EXTI_BUTTON0)
    btn0_irq_hook();
  if (pin == EXTI_BUTTON1)
    btn1_irq_hook();
}

void initApp(void)
{
  // Enable PTI
  configEnablePti();

#if (HAL_I2CSENSOR_ENABLE)
  // Initialize I2C peripheral
  I2CSPM_Init_TypeDef i2cInit = I2CSPM_INIT_DEFAULT;
  I2CSPM_Init(&i2cInit);
#endif // HAL_I2CSENSOR_ENABLE


#if defined(HAL_VCOM_ENABLE)
  // Enable VCOM if requested
  GPIO_PinModeSet(BSP_VCOM_ENABLE_PORT, BSP_VCOM_ENABLE_PIN, gpioModePushPull, HAL_VCOM_ENABLE);
#endif // HAL_VCOM_ENABLE

#if defined(HAL_I2CSENSOR_ENABLE) || defined(HAL_SPIDISPLAY_ENABLE)
#if HAL_I2CSENSOR_ENABLE || HAL_SPIDISPLAY_ENABLE
#define DISPLAY_SENSOR_COMMON_ENABLE 1
#else
#define DISPLAY_SENSOR_COMMON_ENABLE 0
#endif
  //Enable I2C sensor and display if requested
  GPIO_PinModeSet(BSP_I2CSENSOR_ENABLE_PORT, BSP_I2CSENSOR_ENABLE_PIN, gpioModePushPull, DISPLAY_SENSOR_COMMON_ENABLE);
#endif

  GPIO_PinModeSet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, gpioModeInputPull, 1);
  GPIO_PinModeSet(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, gpioModeInputPull, 1);

  GPIOINT_Init();
  GPIOINT_CallbackRegister(EXTI_BUTTON0, gpioInterruptHandler);
  GPIOINT_CallbackRegister(EXTI_BUTTON1, gpioInterruptHandler);
  GPIO_ExtIntConfig(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, EXTI_BUTTON0, false, true, true);
  GPIO_ExtIntConfig(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, EXTI_BUTTON1, false, true, true);
}
