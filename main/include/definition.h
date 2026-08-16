#ifndef _DEFINITION_H_
#define _DEFINITION_H_
#pragma once

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define PRODUCT_NAME            "YOGYUI-MATTER-SCD30"

#define GPIO_PIN_DEFAULT_BTN    0
#define GPIO_PIN_I2C_SCL        2
#define GPIO_PIN_I2C_SDA        1

#define I2C_PORT_NUM            0
#define I2C_MASTER_FREQ         100000

#define TASK_STACK_DEPTH        4096

#define SCD30_USE_DUMMY_DATA    0

#endif