#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_app_gpio.h"
#include "esp_app_http_cfg_page.h"

#define HTTP_CONTENT_200    "OK"

/**
 * \brief      解码URL
 * \param[in]  const char *in   输入
 * \param[out] char       *out  输出
 * \return     无
 */
void http_cfg_url(const char *in, char *out)
{
    int i = 0;
    int j = 0;
    int len = strlen(in);

    while (i < len)
    {
        if (in[i] == '%')
        {
            unsigned char num;
            int num1 = in[i + 1];
            int num2 = in[i + 2];

            if (num1 >= '0' && num1 <= '9')
            {
                num = num1 - '0';
            }
            else if (num1 >= 'A' && num1 <= 'F')
            {
                num = num1 - 'A' + 10;
            }
            else
            {
                out[j++] = in[i++];
                continue;
            }


            if (num2 >= '0' && num2 <= '9')
            {
                num = num * 16 + num2 - '0';
            }
            else if (num2 >= 'A' && num2 <= 'F')
            {
                num = num * 16 + num2 - 'A' + 10;
            }
            else
            {
                out[j++] = in[i++];
                continue;
            }

            out[j++] = num;
            i += 3;
        }
        else
        {
            out[j++] = in[i++];
        }
    }

    out[j] = '\0';
}

/**
 * \brief      任务回调函数
 * \param[in]  void* pvParameters  参数
 * \return     无
 */
static void restart_task(void *pvParameters)
{
    ESP_LOGI(TAG, "------------------restart 2000ms");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "------------------restart now");
    esp_restart();
}

/**
 * \brief      重启
 * \param[out] char          *content       数据体
 * \param[out] uint          *content_len   数据体长度
 * \return     200-成功，其它失败
 */
int http_reboot(char *content, uint *content_len)
{
    xTaskCreate(restart_task, "restart_task", 4096, NULL, 5, NULL);
    strncpy(content, HTTP_CONTENT_200, *content_len);
    *content_len = sizeof(HTTP_CONTENT_200) - 1;
    return 200;
}

/**
 * \brief      配置wifi
 * \param[in]  const char   *param          URL请求参数
 * \param[in]  p_config_wifi wifi           配置数据
 * \param[out] char         *content        数据体
 * \param[out] uint         *content_len    数据体长度
 * \return     200-成功，其它失败
 */
int http_cfg_wifi(const char *param, p_config_wifi wifi, char *content, uint *content_len)
{
    uint type;
    char ssid[32];
    char password[32];
    char format[64];
    char args[1024];

    http_cfg_url(param, args);

    ESP_LOGI(TAG, args);

    snprintf(format, sizeof(format) - 1, "%s=%%%d[^&]&%s=%%%d[^&]&%s=%%d", CONFIG_WIFI_SSID, sizeof(ssid) - 1, CONFIG_WIFI_PASSWORD, sizeof(password) - 1, CONFIG_WIFI_TYPE);

    ESP_LOGI(TAG, format);

    if (3 == sscanf(args, format, ssid, password, &type))
    {
        if ((type == WIFI_TYPE_AP || type == WIFI_TYPE_STA) && (0 != strcmp(ssid, "")) && (0 != strcmp(password, "")))
        {
            wifi->type = type;
            strcpy(wifi->ssid, ssid);
            strcpy(wifi->password, password);

            config_put_wifi(wifi);

            strncpy(content, HTTP_CONTENT_200, *content_len);
            *content_len = sizeof(HTTP_CONTENT_200) -1;
            return 200;
        }
    }

    *content_len = snprintf(content, *content_len, "arg %s %s %s error", CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, CONFIG_WIFI_TYPE);
    ESP_LOGE(TAG, content);
    return 400;
}

/**
 * \brief      配置mqtt
 * \param[in]  const char   *param          URL请求参数
 * \param[in]  p_config_mqtt mqtt           配置数据
 * \param[out] char         *content        数据体
 * \param[out] uint         *content_len    数据体长度
 * \return     0-成功，其它失败
 */
int http_cfg_mqtt(const char *param, p_config_mqtt mqtt, char *content, uint *content_len)
{
    char broker[32];
    char username[32];
    char password[32];
    char clientid[64];
    char topic[MQTT_TOPIC_MAX][64];
    char format[128];
    char args[1024];

    http_cfg_url(param, args);

    ESP_LOGI(TAG, args);

    // username,password多取一个=,当为空串时,sscanf出错
    snprintf(format, sizeof(format) - 1, "%s=%%%d[^&]&%s%%%d[^&]&%s%%%d[^&]&%s=%%%d[^&]&", CONFIG_MQTT_BROKER, sizeof(broker) - 1,
                                                                                           CONFIG_MQTT_USERNAME, sizeof(username) - 1,
                                                                                           CONFIG_MQTT_PASSWORD, sizeof(password) - 1,
                                                                                           CONFIG_MQTT_CLIENTID, sizeof(clientid) - 1);

    if (4 == sscanf(args, format, broker, username, password, clientid)) // 函数将返回成功赋值的字段个数
    {
        char *beg = strstr(args, "&" CONFIG_MQTT_TOPIC);
        char *ptr = beg;
        int i;

        sprintf(format, "&%s=%%%d[^&]&", CONFIG_MQTT_TOPIC, sizeof(topic[0]) - 1);

        for (i = 0; NULL != ptr; i++)
        {
            if (1 != sscanf(ptr, format, topic[i]))
            {
                *content_len = snprintf(content, *content_len, "%s error", CONFIG_MQTT_TOPIC);
                ESP_LOGE(TAG, content);
                return 400;
            }

            ptr = strstr(ptr + 1, "&" CONFIG_MQTT_TOPIC);
        }

        mqtt->topic_count = i;
        strcpy(mqtt->broker,   broker);
        strcpy(mqtt->username, &username[1]);
        strcpy(mqtt->password, &password[1]);
        strcpy(mqtt->clientid, clientid);

        config_put_mqtt(mqtt);

        strncpy(content, HTTP_CONTENT_200, *content_len);
        *content_len = sizeof(HTTP_CONTENT_200) - 1;
        return 200;
    }

    *content_len = snprintf(content, *content_len, "arg %s %s %s %s error", CONFIG_MQTT_BROKER, CONFIG_MQTT_USERNAME, CONFIG_MQTT_PASSWORD, CONFIG_MQTT_CLIENTID);
    ESP_LOGE(TAG, content);
    return 400;
}

/**
 * \brief      配置http
 * \param[in]  const char   *param          URL请求参数
 * \param[in]  p_config_http http           配置数据
 * \param[out] char         *content        数据体
 * \param[out] uint         *content_len    数据体长度
 * \return     200-成功，其它失败
 */
int http_cfg_http(const char *param, p_config_http http, char *content, uint *content_len)
{
    char format[64];
    char args[1024];

    http_cfg_url(param, args);

    ESP_LOGI(TAG, args);

    snprintf(format, sizeof(format) - 1, "%s=%%d", CONFIG_HTTP_PORT);

    if (1 == sscanf(args, format, &(http->port))) // 函数将返回成功赋值的字段个数
    {
        config_put_http(http);

        strncpy(content, HTTP_CONTENT_200, *content_len);
        *content_len = sizeof(HTTP_CONTENT_200) - 1;
        return 200;
    }

    *content_len = snprintf(content, *content_len, "arg %s error", CONFIG_HTTP_PORT);
    ESP_LOGE(TAG, content);
    return 400;
}

/**
 * \brief      配置light
 * \param[in]  const char    *param         URL请求参数
 * \param[in]  p_config_light light         配置数据
 * \param[out] char          *content       数据体
 * \param[out] uint          *content_len   数据体长度
 * \return     200-成功，其它失败
 */
int http_cfg_light(const char *param, p_config_light light, char *content, uint *content_len)
{
    char format[64];
    char args[1024];

    http_cfg_url(param, args);

    ESP_LOGI(TAG, args);

    snprintf(format, sizeof(format) - 1, "%s=%%d", CONFIG_LIGHT_ON);

    if (1 == sscanf(args, format, &(light->on))) // 函数将返回成功赋值的字段个数
    {
        gpio_led(light->on);
        config_put_light(light);

        strncpy(content, HTTP_CONTENT_200, *content_len);
        *content_len = sizeof(HTTP_CONTENT_200) - 1;
        return 200;
    }

    *content_len = snprintf(content, *content_len, "arg %s error", CONFIG_LIGHT_ON);
    ESP_LOGE(TAG, content);
    return 400;
}
