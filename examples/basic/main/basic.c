#include "driver/i2s_types.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/ringbuf.h"
#include "my_dac_component.h"
#include "my_adc_component.h"
#include "freertos/FreeRTOS.h"

#include "esp_check.h"
#include "portmacro.h"
#include <stddef.h>
#include <stdint.h>

#define DATA_BUF_SIZE 64

typedef struct {
    RingbufHandle_t adc_buf_handle;
    RingbufHandle_t dac_buf_handle;
} buf_handles_t;

void mediation_task(void *args){
    buf_handles_t *buf_handles = (buf_handles_t *)args;
    RingbufHandle_t adc_buf_handle = buf_handles->adc_buf_handle;
    size_t r_bytes;

    RingbufHandle_t dac_buf_handle = buf_handles->dac_buf_handle;

    int32_t data[DATA_BUF_SIZE];
    size_t head = 0;

    while(true){
        int32_t *samples = (int32_t *)xRingbufferReceive(adc_buf_handle, &r_bytes, portMAX_DELAY);
        size_t n = r_bytes / sizeof(int32_t);
        for(size_t i = 0; i + 1 < n; i+=2){
            int32_t lin = samples[i] >> 8;
            int32_t rin = samples[i + 1] >> 8;

            if(head>=DATA_BUF_SIZE){
                UBaseType_t res = xRingbufferSend(dac_buf_handle, data, sizeof(data), portMAX_DELAY);
                if(res != pdTRUE){
                    printf("Failed to send data\n");
                }
                head = 0;
            }
            data[head] = lin << 8;
            head++;
        }
        vRingbufferReturnItem(adc_buf_handle, samples);
    }
    

}

void app_main(void)
{
    i2s_chan_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(my_dac_init(&tx_channel));
    ESP_ERROR_CHECK(my_adc_init());

    RingbufHandle_t adc_buf_handle = xRingbufferCreate(1024, RINGBUF_TYPE_NOSPLIT);
    RingbufHandle_t dac_buf_handle = xRingbufferCreate(1024, RINGBUF_TYPE_NOSPLIT);

    buf_handles_t buf_handles = {
        .adc_buf_handle = adc_buf_handle,
        .dac_buf_handle = dac_buf_handle
    };

    my_dac_write_task_args_t my_dac_write_task_args = {
        .buf_handle = dac_buf_handle,
        .tx_channel = tx_channel
    };

    xTaskCreate(my_adc_read_task, "my_adc_read_task", 4096, adc_buf_handle, 5, NULL);
    xTaskCreate(my_dac_write_task, "my_dac_write_task", 4096, &my_dac_write_task_args, 5, NULL);
    xTaskCreate(mediation_task, "mediation_task", 4096, &buf_handles, 5, NULL);
}
