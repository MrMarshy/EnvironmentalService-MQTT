#ifndef MQTT_SSL_USER_H
#define MQTT_SSL_USER_H

#include <stdint.h>
#include "esp_event.h"
#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SemaphoreHandle_t _xMQTT_Semaphore;

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);


esp_mqtt_client_handle_t mqtt_app_start(void);

#ifdef __cplusplus
}
#endif


#endif // MQTT_SSL_USER_H