#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi_types.h"

static const char *TAG = "WIFI_CONNECTOR";

static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
uint8_t successfullyConnected = 0;
esp_netif_t *netif;

static void wifi_disconnect_event(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
{

    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED)
    {
        return;
    }
}

static void wifi_get_ip_event(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    successfullyConnected = 1;
    if (s_semph_get_ip_addrs)
    {
        xSemaphoreGive(s_semph_get_ip_addrs);
    }
    else
    {
        ESP_LOGI(TAG, "- IPv4 address: " IPSTR ",", IP2STR(&event->ip_info.ip));
    }
}

esp_err_t open_wifi_connection(void)
{
    s_semph_get_ip_addrs = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_get_ip_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_disconnect_event, NULL));
    ESP_LOGI(TAG, "Start connect to wifi.");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    esp_netif_config.if_desc = "iot_netif_sta";
    esp_netif_config.route_prio = 128;
    netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold.rssi = -127,
            .threshold.authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    do
    {
        esp_err_t ret = esp_wifi_connect();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "WiFi connect failed! ret:%x", ret);
        }

        ESP_LOGI(TAG, "Waiting for IP(s)");
        xSemaphoreTake(s_semph_get_ip_addrs, 10000);
    } while (successfullyConnected != 1);

    ESP_LOGI(TAG, "WiFi connect success!");
    return ESP_OK;
}