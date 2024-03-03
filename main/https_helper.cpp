#include "https_helper.h"
#include "esp_log.h"

static const char *TAG = "https_config";

void HTTPS_Helper::JSON_Analyze(const cJSON * const root) const {
    double hPa = 0.0f;
	//ESP_LOGI(TAG, "root->type=%s", JSON_Types(root->type));
	cJSON *current_element = NULL;
	//ESP_LOGI(TAG, "roo->child=%p", root->child);
	//ESP_LOGI(TAG, "roo->next =%p", root->next);
	cJSON_ArrayForEach(current_element, root) {
		//ESP_LOGI(TAG, "type=%s", JSON_Types(current_element->type));
		//ESP_LOGI(TAG, "current_element->string=%p", current_element->string);
		if (current_element->string) {
			const char* string = current_element->string;
			ESP_LOGI(TAG, "[%s]", string);
		}
		if (cJSON_IsInvalid(current_element)) {
			ESP_LOGI(TAG, "Invalid");
		} else if (cJSON_IsFalse(current_element)) {
			ESP_LOGI(TAG, "False");
		} else if (cJSON_IsTrue(current_element)) {
			ESP_LOGI(TAG, "True");
		} else if (cJSON_IsNull(current_element)) {
			ESP_LOGI(TAG, "Null");
		} else if (cJSON_IsNumber(current_element)) {
			int valueint = current_element->valueint;
			double valuedouble = current_element->valuedouble;
			ESP_LOGI(TAG, "int=%d double=%f", valueint, valuedouble);

		} else if (cJSON_IsString(current_element)) {
			const char* valuestring = current_element->valuestring;
			ESP_LOGI(TAG, "%s", valuestring);
		} else if (cJSON_IsArray(current_element)) {
			ESP_LOGI(TAG, "Array");
			JSON_Analyze(current_element);
		} else if (cJSON_IsObject(current_element)) {
			ESP_LOGI(TAG, "Object");
			JSON_Analyze(current_element);
		} else if (cJSON_IsRaw(current_element)) {
			ESP_LOGI(TAG, "Raw(Not support)");
		}
	}
}

void HTTPS_Helper::get_json_value_as_double(const char* obj_str, const char* value_str, double *value) const{
    if(_json_root == nullptr){
        ESP_LOGI(TAG, "json root is not initialised");
    }
    const cJSON *obj = cJSON_GetObjectItemCaseSensitive(_json_root, obj_str);
    const cJSON *element = NULL;
    cJSON_ArrayForEach(element, obj){
        cJSON *result = cJSON_GetObjectItemCaseSensitive(element, value_str);
        const cJSON *data = NULL;
        ESP_LOGI(TAG, "result->string: %s", result->string);
        if(cJSON_IsObject(result)){
            cJSON_ArrayForEach(data, result){
                if((cJSON_IsNumber(data)) && (strncmp(data->string, "hpa", strlen("hpa")) == 0)){
                    ESP_LOGI(TAG, "data %s is %f", data->string, data->valuedouble);
                    *value = data->valuedouble;
                    return;
                }
            }
            
        }
    }
}