#ifndef WIFI_SECURE_USER_H
#define WIFI_SECURE_USER_H

#include "cJSON.h"

void https_get_task(void *pvParams);

void https_client(void* user_data);

#ifdef __cplusplus
extern "C" {
#endif

extern SemaphoreHandle_t _xHTTPS_Semaphore;

#ifdef __cplusplus
}
#endif

#endif // WIFI_SECURE_USER_H