#ifndef HTTPS_CONFIG_USER_H
#define HTTPS_CONFIG_USER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

class HTTPS_Helper {
public:
    HTTPS_Helper() = default;

    ~HTTPS_Helper(){
        if(_json_root){
            cJSON_Delete(_json_root);
        }
    };

    char* get_server(){
        return _server;
    }

    int get_port() const{
        return _port;
    }

    char* get_port_str(){
        return _port_as_str;
    }

    char* get_write_req(){
        return _write_req;
    }

    char* get_req(){
        return _get_req;
    }

    char* get_buffer(){
        return _buffer;
    }

    bool use_json() const{
        return _use_json;
    }

    void set_extra_params(void* params){
        _extra_params = params;
    }

    template<class T>
    T* get_extra_params() const{
        return (T*)_extra_params;
    }

    void JSON_Analyze(const cJSON *root) const;

    void get_json_value_as_double(const char* obj_str, const char* value_str, double *result) const;

    cJSON* get_json_root() const{
        return _json_root;
    }

    void set_json_root(cJSON *obj){
        if(obj){
            _json_root = obj;
        }
    }

    virtual void httpsConfigInit(const char* server, const char* api_key, bool use_json) = 0;

protected:
    char _server[32];
    int _port{443};
    char _port_as_str[8];
    char _api_key[64];
    char _write_req[256];
    char _get_req[256];
    char _buffer[2048];
    bool _use_json;
    void* _extra_params;
    cJSON  *_json_root;
private:

};

class HTTPS_CheckWX: public HTTPS_Helper{

public:
    
    HTTPS_CheckWX(){
        HTTPS_CheckWX("EGLL"); // Default to London Heathrow
    }
    
    HTTPS_CheckWX(const char* icao) {
        set_icao(icao);
    }

    ~HTTPS_CheckWX() = default;

    void set_icao(const char* icao){
        snprintf(_icao, sizeof(_icao)-1, "%s", icao);
        _icao[sizeof(_icao)-1] = '\0';
    }

    virtual void httpsConfigInit(const char* server, const char* api_key, bool use_json){

        _use_json = use_json;

        memcpy(_server, server, sizeof(_server)-1);
        _server[sizeof(_server)-1] = '\0';

        memcpy(_api_key, api_key, sizeof(_api_key)-1);
        _api_key[sizeof(_api_key)-1] = '\0';

        snprintf(_get_req, sizeof(_get_req), "https://%s/metar/%s/decoded?x-api-key=%s", _server, _icao, _api_key);

        snprintf(_port_as_str, sizeof(_port_as_str), "%d", _port);

        snprintf(_get_req, sizeof(_get_req), "/metar/%s/decoded?x-api-key=%s", _icao, _api_key);
    }

protected:

private:
    char _icao[8];

};

#endif // HTTPS_CONFIG_USER_H