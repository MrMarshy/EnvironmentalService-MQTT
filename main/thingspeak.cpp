#include "thingspeak.h"
#include <esp_log.h>

static const char *TAG = "THINGSPEAK";

void ThingSpeak::sendToField(int field_num, const char* value) const{
    return;
}

void ThingSpeakMQTT::publishToField(const char* topic, int field_num, const char* value){
    setTopic(topic);
    publishToField(field_num, value);
}

void ThingSpeakMQTT::publishToField(const char* topic, int field_num, const char* value, RETAIN_MODE retain, QOS_MODE qos){
    setTopic(topic);
    publishToField(field_num, value, retain, qos);
}

void ThingSpeakMQTT::publishToField(int field_num, const char* value) const{
    char data_buf[64];
    memset(data_buf, '0', sizeof(data_buf));

    snprintf(data_buf, sizeof(data_buf)-1, "&field%d=%s", field_num, value);

    ESP_LOGI(TAG, "preparing to publish %s%s", _topic, data_buf);

    if(!_mqtt_client){
        ESP_LOGE(TAG, "mqtt_client appears to be unitialised in this context");
        return;
    }

    int msg_id = esp_mqtt_client_publish(_mqtt_client, 
        _topic, data_buf, 0, (int)_qos, (int)_retain);

    ESP_LOGI(TAG, "sent publish successful: %s (%s%s)", msg_id != -1 ? "True" : "False", _topic, data_buf);
}

void ThingSpeakMQTT::publishToField(int field_num, const char* value, RETAIN_MODE retain, QOS_MODE qos){
    
    _qos = qos;
    _retain = retain;

    publishToField(field_num, value);
}

void ThingSpeakMQTT::publishToAllFields(const char* topic, char** values, int len_values, RETAIN_MODE retain, QOS_MODE qos){
    setTopic(topic);
    publishToAllFields(values, len_values, retain, qos);
}

void ThingSpeakMQTT::publishToAllFields(char** values, int len_values, RETAIN_MODE retain, QOS_MODE qos){
    _qos = qos;
    _retain = retain;

    publishToAllFields(values, len_values);
}


void ThingSpeakMQTT::publishToAllFields(char** values, int len_values) const{

    char data_buf[64];
    char tmp_buf[64];
    memset(data_buf, '\0', sizeof(data_buf));
    memset(tmp_buf, '\0', sizeof(tmp_buf));

    if(len_values <= _num_fields){
        for(int i = 0; i < _num_fields; ++i){
            snprintf(tmp_buf, sizeof(tmp_buf)-1, "&field%d=%s", i+1, values[i]);
            strncat(data_buf, tmp_buf, strlen(tmp_buf));
        }
    }

    // ESP_LOGI(TAG, "preparing to publish to all fields: %s%s", _topic, data_buf);

    int msg_id = esp_mqtt_client_publish(_mqtt_client, 
        _topic, data_buf, 0, (int)_qos, (int)_retain);

    // ESP_LOGI(TAG, "sent publish to all fields successful: %s %s%s", msg_id != -1 ? "True" : "False", _topic, data_buf);
}

void ThingSpeakMQTT::setTopic(const char* topic){
    memcpy(_topic, topic, sizeof(_topic)-1);
    _topic[sizeof(_topic)-1] = '\0';
}