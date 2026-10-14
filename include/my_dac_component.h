#include "esp_err.h"
#include "driver/i2s_common.h"
#include "driver/i2s_types.h"
#include "freertos/ringbuf.h"

typedef struct {
    RingbufHandle_t buf_handle;
    i2s_chan_handle_t tx_channel;
} my_dac_write_task_args_t;


esp_err_t my_dac_init(i2s_chan_handle_t *tx_channel);
void my_dac_write_task(void *args);