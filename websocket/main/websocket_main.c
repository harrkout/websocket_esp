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
#include <stdlib.h>
#include <time.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define SERVER_URI "ws://192.168.1.10:8081"

static const char *TAG = "esp32_client";
static esp_websocket_client_handle_t client;

static bool send_immediate_data = false;

void simulate_sensor_data(char* data_buffer) {
    float humidity = (rand() % 101);
    float soil_moisture = (rand() % 101);
    snprintf(data_buffer, 100, "{\"humidity\": %.2f, \"soil_moisture\": %.2f}", humidity, soil_moisture);
}

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected");
            break;
        case WEBSOCKET_EVENT_DATA:
            if (data->op_code == 0x8) {
                ESP_LOGW(TAG, "Closed message code=%d", data->data_ptr[0] << 8 | data->data_ptr[1]);
            } else {
                if (strncmp((char *)data->data_ptr, "GET_DATA", data->data_len) == 0) {
                    send_immediate_data = true;
                }
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGI(TAG, "Error");
            break;
    }
}

void websocket_task(void *pvParameters)
{
    char message[100];
    TickType_t last_sent_time = 0;
    while (1) {
        if (esp_websocket_client_is_connected(client)) {
            if (send_immediate_data) {
                simulate_sensor_data(message);
                esp_websocket_client_send_text(client, message, strlen(message), portMAX_DELAY);
                send_immediate_data = false;
            } else {
                TickType_t current_time = xTaskGetTickCount();
                if (current_time - last_sent_time >= pdMS_TO_TICKS(10000)) {
                    simulate_sensor_data(message);
                    esp_websocket_client_send_text(client, message, strlen(message), portMAX_DELAY);
                    last_sent_time = current_time;
                }
            }
        } else {
            ESP_LOGW(TAG, "WebSocket not connected, retrying...");
        }
        vTaskDelay(100);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    const esp_websocket_client_config_t websocket_cfg = {
        .uri = SERVER_URI,
    };

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    esp_websocket_client_start(client);

    srand(time(NULL));

    xTaskCreate(&websocket_task, "websocket_task", 4096, NULL, 5, NULL);
}
