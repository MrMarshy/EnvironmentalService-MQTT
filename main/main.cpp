#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_system.h"
#include <esp_log.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"

#include "wifi_sta.h"
#include "wifi_secure.h"
#include "mqtt_ssl.h"

#include "https_helper.h"
#include "thingspeak.h"
#include "bme680.h"
#include "math.h"

/* MQTT over SSL to Thingspeak Example */
static const char *TAG = "APP_MAIN";

static QueueHandle_t hpa_queue;
static TimerHandle_t pressure_timer_update;
static esp_mqtt_client_handle_t mqtt_client;

extern "C" void app_main();
extern "C" void ess_task(void *pvParams);
extern "C" void metar_task(void *pvParams);
extern "C" void vTimerCallbackOneHourExpired(TimerHandle_t pxTimer);

#define PORT ((i2c_port_t)0)

#define BME680_I2C_MASTER_SCL ((gpio_num_t)CONFIG_BME680_I2C_MASTER_SCL)
#define BME680_I2C_MASTER_SDA ((gpio_num_t)CONFIG_BME680_I2C_MASTER_SDA)

#if defined(CONFIG_BME680_I2C_ADDR_0)
#define ADDR BME680_I2C_ADDR_0
#else
#define ADDR BME680_I2C_ADDR_1
#endif

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

static float moving_average_filter_pressure(float a, bool reset);


void app_main(){

	ESP_LOGI(TAG, "[APP] Startup");
	ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

	esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

	ESP_ERROR_CHECK(nvs_flash_init());

	wifi_init_sta();

    hpa_queue = xQueueCreate(1, sizeof(double));

	mqtt_client = mqtt_app_start();

    if(mqtt_client){
        /* Create an environmental service task*/
	    xTaskCreate(ess_task, "ess_sender_task", 8192, NULL, 1, NULL);

        /* Create get METAR data task to retrieve hPa value in current location. */
        xTaskCreate(metar_task, "metar_task", 8192, NULL, 1, NULL);
    }
    else{
        ESP_LOGI(TAG, "Unable to initialise mqtt_client");
    }
}


void ess_task(void *pvParams){
    (void)pvParams;

    double hpa_val_metar = -999.0;

    /* Wait until we get hpa value from METAR */
    xQueueReceive(hpa_queue, &hpa_val_metar, portMAX_DELAY);

    /* Create timer that callback every hour to update the nearby airport hPa */
    pressure_timer_update = xTimerCreate(
        "timer_one_hour",
        pdMS_TO_TICKS(3.6e+6), // 1 hour
        pdTRUE, // auto reload
        (void*)0, // timer id
        vTimerCallbackOneHourExpired
    );

    xTimerStart(pressure_timer_update, 0);

    bme680_t sensor;
    memset(&sensor, 0, sizeof(bme680_t));

    ESP_ERROR_CHECK(bme680_init_desc(&sensor, ADDR, PORT, BME680_I2C_MASTER_SDA, BME680_I2C_MASTER_SCL));

    /* Init the bme680 sensor */
    ESP_ERROR_CHECK(bme680_init_sensor(&sensor));

    // Changes the oversampling rates to 4x oversampling for temperature
    // and 2x oversampling for humidity. and 2x oversampling for pressure.
    bme680_set_oversampling_rates(&sensor, BME680_OSR_4X, BME680_OSR_2X, BME680_OSR_2X);

    // Change the IIR filter size for temperature and pressure to 7.
    bme680_set_filter_size(&sensor, BME680_IIR_SIZE_7);

    // Change the heater profile 0 to 200 degree Celsius for 100 ms.
    bme680_set_heater_profile(&sensor, 0, 200, 100);
    bme680_use_heater_profile(&sensor, 0);

    // Set ambient temperature to the current temperature in degree Celsius
    bme680_set_ambient_temperature(&sensor, 11);

    // as long as sensor configuration isn't changed, duration is constant
    uint32_t duration;
    bme680_get_measurement_duration(&sensor, &duration);

    TickType_t last_wakeup = xTaskGetTickCount();

    bme680_values_float_t values;

    ThingSpeakMQTT thingspeak_hndl = ThingSpeakMQTT(mqtt_client, 4);
    thingspeak_hndl.setTopic("channels/" CONFIG_THINGSPEAK_CHANNEL_ID "/publish");

    char* values_as_str [4] = {0}; 
    char temperature_as_str[8] = {0};
    char pressure_as_str[8] = {0};
    char humidity_as_str[8] = {0};
    char gas_resistance_as_str[16] = {0};

    while(1){

        if(xSemaphoreTake(_xMQTT_Semaphore, portMAX_DELAY) == pdTRUE){

            if(bme680_force_measurement(&sensor) == ESP_OK){
                // passive waiting until measurement results are available
                vTaskDelay(duration);

                // Get BME680 measurement results and publish to Thingspeak over Secure MQTT
                if(bme680_get_results_float(&sensor, &values) == ESP_OK){
                    ESP_LOGI(TAG, "Temperature (degC): %.2f", values.temperature);
                    ESP_LOGI(TAG, "Humidity (%%): %.2f", values.humidity);
                    // ESP_LOGI(TAG, "Pressure raw (hPa): %.5f", values.pressure);

                    /* Check if there is an updated hpa value from metar */
                    xQueueReceive(hpa_queue, &hpa_val_metar, pdMS_TO_TICKS(10));

                    
                    // P0 (1013.25) is the mean sea pressure level, you should change this to the 
                    // current local value for your area.
                    values.pressure = moving_average_filter_pressure(values.pressure, false);
                    ESP_LOGI(TAG, "Pressure filtered (hPa): %.5f", values.pressure);
                    values.pressure = (values.pressure + (float)hpa_val_metar) / 2.0f;
                    ESP_LOGI(TAG, "Pressure (hPa): %.5f", values.pressure);
                    ESP_LOGI(TAG, "METAR Pressure %.2f", (float)hpa_val_metar);
                    ESP_LOGI(TAG, "Gas resistance (ohms): %.2f", values.gas_resistance);
                }

                snprintf(temperature_as_str, sizeof(temperature_as_str)-1, "%.2f", values.temperature);
                snprintf(humidity_as_str, sizeof(humidity_as_str)-1, "%.2f", values.humidity);
                snprintf(pressure_as_str, sizeof(pressure_as_str)-1, "%.2f", values.pressure);
                snprintf(gas_resistance_as_str, sizeof(gas_resistance_as_str)-1, "%.2f", values.gas_resistance);

                values_as_str[0] = temperature_as_str;
                values_as_str[1] = humidity_as_str;
                values_as_str[2] = pressure_as_str;
                values_as_str[3] = gas_resistance_as_str;
                thingspeak_hndl.publishToAllFields(values_as_str, 4);
                xSemaphoreGive(_xMQTT_Semaphore);
            }
        }

        xTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(10000));
    }
}

void metar_task(void *pvParams){
    HTTPS_CheckWX check_ws = HTTPS_CheckWX(CONFIG_CHECKWX_ICAO);

    check_ws.httpsConfigInit(CONFIG_CHECKWX_URL, CONFIG_CHECKWX_API_KEY, true);
    
    ESP_LOGI(TAG, "%s", check_ws.get_port_str());
    ESP_LOGI(TAG, "%s", check_ws.get_server());
    ESP_LOGI(TAG, "%s", check_ws.get_req());

    https_client(&check_ws);

    double hPa = -999.0;

    while(1){
        if(xSemaphoreTake(_xHTTPS_Semaphore, portMAX_DELAY) == pdTRUE){

            check_ws.get_json_value_as_double("data", "barometer", &hPa);

            ESP_LOGI(TAG, "Metar Task got hPa of %f", hPa);

            xQueueSend(hpa_queue, &hPa, 0);

            xSemaphoreGive(_xHTTPS_Semaphore);

            vTaskDelete(NULL);
        }
    }
}

void vTimerCallbackOneHourExpired(TimerHandle_t pxTimer){
    /* Create get METAR data task to retrieve hPa value in current location. */
    ESP_LOGI(TAG, "Updating pressure value from metar!");
    xTaskCreate(metar_task, "metar_task", 8192, NULL, 1, NULL);
}

/* not threadsafe! */
static float moving_average_filter_pressure(float next, bool reset){
    static float sum = 0.0f;
    static float prev = 0.0f;
    static int len = 5;
    static int pos = 0;
    static float arr[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    if(reset){
        sum = 0.0f;
        prev = 0.0f;
        pos = 0;
        for(int i = 0; i < len; ++i){
            arr[i] = 0.0f;
        }
    }

    if(pos == 5){
        pos = 0;
    }

    sum = sum - arr[pos] + next;

    arr[pos] = next;

    pos++;

    return sum / (float)len;
}
