#ifndef THINGSPEAK_USER_H
#define THINGSPEAK_USER_H

#include "mqtt_client.h"

class ThingSpeak{

public:
    ThingSpeak()=default;
    ~ThingSpeak()=default;

    void sendToAllFields() const;
    void sendToField(int field_num, const char* value) const;

protected:

private:

};

class ThingSpeakMQTT: public ThingSpeak{
public:
    enum class QOS_MODE{
        AT_MOST_ONCE = 0,
        AT_LEAST_ONCE = 1,
        EXACTLY_ONCE = 2,
    };

    enum class RETAIN_MODE{
        DONT_RETAIN,
        DO_RETAIN,
    };

    ThingSpeakMQTT(esp_mqtt_client_handle_t mqtt_client): 
        _mqtt_client{mqtt_client}, _qos{QOS_MODE::AT_LEAST_ONCE}, 
        _retain{RETAIN_MODE::DO_RETAIN}, _num_fields{0} {}

    ThingSpeakMQTT(esp_mqtt_client_handle_t &mqtt_client, int num_fields): 
        _mqtt_client{mqtt_client}, _qos{QOS_MODE::AT_LEAST_ONCE}, 
        _retain{RETAIN_MODE::DO_RETAIN} {

            setNumFields(num_fields);
        }

    ThingSpeakMQTT(esp_mqtt_client_handle_t mqtt_client, QOS_MODE qos, RETAIN_MODE retain, int num_fields): 
        _mqtt_client{mqtt_client}, _qos{qos}, 
        _retain{retain}, _num_fields{num_fields} {}

    ~ThingSpeakMQTT() = default;

    void setMQTTClient(esp_mqtt_client_handle_t &mqtt_client){
        _mqtt_client = mqtt_client;
    }

    esp_mqtt_client_handle_t getMQTTClient(){
        return _mqtt_client;
    }

    void publishToAllFields(const char* topic, char** values, int len_values, RETAIN_MODE retain, QOS_MODE qos);
    void publishToAllFields(char** values, int len_values, RETAIN_MODE retain, QOS_MODE qos);
    void publishToAllFields(char** values, int len_values) const;
    void publishToField(const char* topic, int field_num, const char* value, RETAIN_MODE retain, QOS_MODE qos);
    void publishToField(const char* topic, int field_num, const char* value);
    void publishToField(int field_num, const char* value, RETAIN_MODE retain, QOS_MODE qos);
    void publishToField(int field_num, const char* value) const;

    void setTopic(const char* topic);

    void setQOS(QOS_MODE qos){
        _qos = qos;
    }

    void setRetain(RETAIN_MODE retain){
        _retain = retain;
    }

    void setNumFields(int num_fields){
        assert(num_fields > 0);
        _num_fields = num_fields;
    }

protected:

private:
    esp_mqtt_client_handle_t _mqtt_client;
    QOS_MODE _qos;
    RETAIN_MODE _retain;
    int _num_fields;
    char _topic[32];
};
 
#endif
