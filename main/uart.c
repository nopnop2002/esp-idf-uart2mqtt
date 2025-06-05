/*	UART Example

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
#include "freertos/message_buffer.h"
#include "driver/uart.h"
#include "esp_log.h"

extern MessageBufferHandle_t xMessageBufferRx;
extern MessageBufferHandle_t xMessageBufferTx;
extern size_t xItemSize;

void uart_tx(void* pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start using GPIO%d", CONFIG_UART_TX_GPIO);

	char buffer[xItemSize];
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferTx, buffer, sizeof(buffer), portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			ESP_LOGD(pcTaskGetName(NULL), "xMessageBufferReceive buffer=[%.*s]",received, buffer);
			ESP_LOG_BUFFER_HEXDUMP(pcTaskGetName(NULL), buffer, received, ESP_LOG_INFO);
			int txBytes = uart_write_bytes(UART_NUM_1, buffer, received);
			if (txBytes != received) {
				ESP_LOGE(pcTaskGetName(NULL), "uart_write_bytes Fail. txBytes=%d received=%d", txBytes, received);
			}
		}
	} // end while

	// Never reach here
	vTaskDelete(NULL);
}

void uart_rx(void* pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start using GPIO%d", CONFIG_UART_RX_GPIO);

	char buffer[xItemSize];
	while (1) {
		int received = uart_read_bytes(UART_NUM_1, buffer, xItemSize, 10 / portTICK_PERIOD_MS);
		// There is some rxBuf in rx buffer
		if (received > 0) {
			ESP_LOGI(pcTaskGetName(NULL), "received=%d", received);
			ESP_LOGD(pcTaskGetName(NULL), "buffer=[%.*s]",received, buffer);
			ESP_LOG_BUFFER_HEXDUMP(pcTaskGetName(NULL), buffer, received, ESP_LOG_INFO);
			size_t sended = xMessageBufferSend(xMessageBufferRx, buffer, received, 100);
			if (sended != received) {
				ESP_LOGE(pcTaskGetName(NULL), "xMessageBufferSend fail received=%d sended=%d", received, sended);
				break;
			}
		} else {
			// There is no data in rx buufer
			//ESP_LOGI(pcTaskGetName(NULL), "Read %d", rxBytes);
		}
	} // end while

	// Stop connection
	ESP_LOGI(pcTaskGetName(NULL), "Task Delete");
	vTaskDelete(NULL);
}
