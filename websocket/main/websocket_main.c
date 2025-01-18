#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_websocket_client.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#define SERVER_URI "ws://ws.ifelse.io"

static const char *TAG = "esp32_client";
static esp_websocket_client_handle_t client;

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
            if (data->op_code == 0x8) {
                ESP_LOGW(TAG, "Received closed message with code=%d", data->data_ptr[0] << 8 | data->data_ptr[1]);
            } else {
                ESP_LOGI(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
    }
}

void websocket_task(void *pvParameters)
{
    const char *message = "Hello from ESP32";
    while (1) {
        if (esp_websocket_client_is_connected(client)) {
            esp_websocket_client_send_text(client, message, strlen(message), portMAX_DELAY);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    const esp_websocket_client_config_t websocket_cfg = {
        .uri = "ws://ws.ifelse.io",    
    };

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    esp_websocket_client_start(client);

    xTaskCreate(&websocket_task, "websocket_task", 4096, NULL, 5, NULL);
}
