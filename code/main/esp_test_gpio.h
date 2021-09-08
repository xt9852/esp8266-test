#pragma once // 只编译一次


/**
 * \brief      led灯,1秒间隔闪灯
 * \return     无
 */
void gpio_led_init();

/**
 * \brief      初始化74LS165使用的GPIO
 * \return     无
 */
void gpio_cpu_init();

/**
 * \brief      载入74LS165数据
 * \return     无
 */
void gpio_cpu_load_data();

/**
 * \brief      读取74LS165数据,组装成数据
 * \param[in]  int count   组装数据的个数
 * \return     数据
 */
uint gpio_cpu_get_data(uint count);

/**
 * \brief      输出数据
 * \param[in]  uint clk     心跳
 * \param[in]  uint int0    中断0
 * \param[in]  uint int1    中断1
 * \return     无
 */
void gpio_cpu_set_data(uint clk, uint int0, uint int1);
