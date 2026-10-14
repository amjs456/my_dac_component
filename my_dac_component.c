#include "my_dac_component.h"
#include "driver/i2s_std.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "hal/i2s_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <stdint.h>

static const char *TAG = "my_adc_component";


esp_err_t my_dac_init(i2s_chan_handle_t *tx_channel){
    i2s_chan_config_t channel_conf = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    channel_conf.auto_clear_before_cb = true;

    esp_err_t err = i2s_new_channel(&channel_conf, tx_channel, NULL);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "i2s_new_channel failed");
        return err;
    }

    const i2s_std_config_t std_conf = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_I2S_OUTPUT_BCLK_GPIO,
            .ws = CONFIG_I2S_OUTPUT_WS_GPIO,
            .dout = CONFIG_I2S_OUTPUT_DOUT_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .bclk_inv = false,
                .mclk_inv = false,
                .ws_inv = false
            }
        }
    };

    err = i2s_channel_init_std_mode(*tx_channel, &std_conf);
    if(err != ESP_OK){
        i2s_del_channel(*tx_channel);
        *tx_channel = NULL;
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed");
        return err;
    }

    err = i2s_channel_enable(*tx_channel);
    if(err != ESP_OK){
        i2s_del_channel(*tx_channel);
        ESP_LOGE(TAG, "i2s_channel_enable failed");
        return err;
    }

    return ESP_OK;
}

static esp_err_t i2s_write(i2s_chan_handle_t tx_channel, const void *data, size_t data_size){
    const uint8_t *next = (const uint8_t *)data;
    size_t remaining = data_size;

    while(remaining > 0){
        size_t bytes_written = 0;
        ESP_ERROR_CHECK(
            i2s_channel_write(tx_channel, next, remaining, &bytes_written, portMAX_DELAY)
        );
        if(bytes_written == 0){
            return ESP_ERR_INVALID_SIZE;
        }
        next += bytes_written;
        remaining -= bytes_written;
    }
    return ESP_OK;
}

void my_dac_write_task(void *args){
    my_dac_write_task_args_t *my_dac_write_task_args = (my_dac_write_task_args_t *)args;
    RingbufHandle_t buf_handle = my_dac_write_task_args->buf_handle;
    i2s_chan_handle_t tx_channel = my_dac_write_task_args->tx_channel;
    size_t r_bytes;

    while(true){
        int32_t *data = (int32_t *)xRingbufferReceive(buf_handle, &r_bytes, portMAX_DELAY);
        i2s_write(tx_channel, data, r_bytes);
        vRingbufferReturnItem(buf_handle, data);
    }
}