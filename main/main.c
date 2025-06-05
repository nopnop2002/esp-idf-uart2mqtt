/*
	UART - MQTT bridge Example

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "mdns.h"

MessageBufferHandle_t xMessageBufferRx;
MessageBufferHandle_t xMessageBufferTx;

// The total number of bytes (not messages) the message buffer will be able to hold at any one time.
size_t xBufferSizeBytes = 1024;
// The size, in bytes, required to hold each item in the message,
size_t xItemSize = 64;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "MAIN";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

esp_err_t wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_ESP_WIFI_SSID,
			.password = CONFIG_ESP_WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	//ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	esp_err_t ret_value = ESP_OK;
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
		WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
		pdFALSE,
		pdFALSE,
		portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
		ret_value = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
		ret_value = ESP_FAIL;
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
	return ret_value;
}

#define RX_BUF_SIZE 128

void uart_init(void) {
	const uart_config_t uart_config = {
		//.baud_rate = 115200,
		.baud_rate = CONFIG_UART_BAUD_RATE,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		.source_clk = UART_SCLK_DEFAULT,
#else
		.source_clk = UART_SCLK_APB,
#endif
	};
	// We won't use a buffer for sending data.
	uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART_NUM_1, &uart_config);
	//uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	uart_set_pin(UART_NUM_1, CONFIG_UART_TX_GPIO, CONFIG_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

esp_err_t query_mdns_host(const char * host_name, char *ip)
{
	ESP_LOGD(__FUNCTION__, "Query A: %s", host_name);

	struct esp_ip4_addr addr;
	addr.addr = 0;

	esp_err_t err = mdns_query_a(host_name, 10000,	&addr);
	if(err){
		if(err == ESP_ERR_NOT_FOUND){
			ESP_LOGW(__FUNCTION__, "%s: Host was not found!", host_name);
		} else {
			ESP_LOGE(__FUNCTION__, "Query Failed: %s", esp_err_to_name(err));
		}
		return ESP_FAIL;
	}

	ESP_LOGD(__FUNCTION__, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
	sprintf(ip, IPSTR, IP2STR(&addr));
	return ESP_OK;
}

void convert_mdns_host(char * from, char * to)
{
	ESP_LOGI(__FUNCTION__, "from=[%s]",from);
	strcpy(to, from);
	char *sp;
	sp = strstr(from, ".local");
	if (sp == NULL) return;

	int _len = sp - from;
	ESP_LOGD(__FUNCTION__, "_len=%d", _len);
	char _from[128];
	strcpy(_from, from);
	_from[_len] = 0;
	ESP_LOGI(__FUNCTION__, "_from=[%s]", _from);

	char _ip[128];
	esp_err_t ret = query_mdns_host(_from, _ip);
	ESP_LOGI(__FUNCTION__, "query_mdns_host=%d _ip=[%s]", ret, _ip);
	if (ret != ESP_OK) return;

	strcpy(to, _ip);
	ESP_LOGI(__FUNCTION__, "to=[%s]", to);
}

void mqtt_sub(void *pvParameters);
void mqtt_pub(void *pvParameters);
void uart_tx(void *pvParameters);
void uart_rx(void *pvParameters);

void app_main(void)
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	wifi_init_sta();

	// Initialize UART
	uart_init();

	// Initialize mDNS
	ESP_ERROR_CHECK( mdns_init() );

	// Create MessageBuffer
	xMessageBufferRx = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferRx );
	xMessageBufferTx = xMessageBufferCreate(xBufferSizeBytes);
	configASSERT( xMessageBufferTx );

	// Start MQTT Subscribe
	xTaskCreate(mqtt_sub, "SUB", 1024*4, NULL, 2, NULL);

	// Start MQTT Publish
	xTaskCreate(mqtt_pub, "PUB", 1024*4, NULL, 2, NULL);

	// Start UART TX
	xTaskCreate(uart_tx, "UART-TX", 1024*3, NULL, 2, NULL);

	// Start UART RX
	xTaskCreate(uart_rx, "UART-RX", 1024*3, NULL, 2, NULL);
}
