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

extern MessageBufferHandle_t xMessageBufferMqtt;
extern MessageBufferHandle_t xMessageBufferUart;
extern size_t xItemSize;

void traceHeap() {
    static uint32_t _free_heap_size = 0;
    if (_free_heap_size == 0) _free_heap_size = esp_get_free_heap_size();

    int _diff_free_heap_size = _free_heap_size - esp_get_free_heap_size();
    ESP_LOGI(__FUNCTION__, "_diff_free_heap_size=%d", _diff_free_heap_size);
    ESP_LOGI(__FUNCTION__, "esp_get_free_heap_size() : %6"PRIu32"\n", esp_get_free_heap_size() );
#if 0
    printf("esp_get_minimum_free_heap_size() : %6"PRIu32"\n", esp_get_minimum_free_heap_size() );
    printf("xPortGetFreeHeapSize() : %6zd\n", xPortGetFreeHeapSize() );
    printf("xPortGetMinimumEverFreeHeapSize() : %6zd\n", xPortGetMinimumEverFreeHeapSize() );
    printf("heap_caps_get_free_size(MALLOC_CAP_32BIT) : %6d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT) );
    // that is the amount of stack that remained unused when the task stack was at its greatest (deepest) value.
    printf("uxTaskGetStackHighWaterMark() : %6d\n", uxTaskGetStackHighWaterMark(NULL));
#endif
}

void uart_tx(void* pvParameters)
{
	ESP_LOGI(pcTaskGetName(NULL), "Start using GPIO%d", CONFIG_UART_TX_GPIO);

	char buffer[xItemSize];
	while(1) {
		size_t received = xMessageBufferReceive(xMessageBufferUart, buffer, sizeof(buffer), portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive received=%d", received);
		if (received > 0) {
			ESP_LOGI(pcTaskGetName(NULL), "xMessageBufferReceive buffer=[%.*s]",received, buffer);
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
			traceHeap();
			ESP_LOGI(pcTaskGetName(NULL), "received=%d", received);
			ESP_LOGI(pcTaskGetName(NULL), "buffer=[%.*s]",received, buffer);
			size_t sended = xMessageBufferSend(xMessageBufferMqtt, buffer, received, 100);
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
