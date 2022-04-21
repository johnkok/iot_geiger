#include "display.h"
#include "fonts.h"

static const char *TAG = "DISPLAY";

int i2c_master_port = 0;
i2c_config_t conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = 23, //I2C_MASTER_SDA_IO,         // select GPIO specific to your project
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_io_num = 21, //I2C_MASTER_SCL_IO,         // select GPIO specific to your project
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 100000, //I2C_MASTER_FREQ_HZ,  // select frequency specific to your project
//  .clk_flags = 0,          !< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here.
};
static uint8_t display_ram[8][128];

static esp_err_t __attribute__((unused)) i2c_master_write_slave(i2c_port_t i2c_num, uint8_t *data_wr, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, 0x78, 1);
    i2c_master_write(cmd, data_wr, size, 1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void display_update(uint8_t *ram)
{
    unsigned char m,n;
    uint8_t buffer[2];

    for(m=0;m<8;m++)
    {

        buffer[0] = 0;
        buffer[1] = 0xB0 + m;
        i2c_master_write_slave(0, buffer, 2);
        buffer[1] = 0x00;
        i2c_master_write_slave(0, buffer, 2);
        buffer[1] = 0x10;
        i2c_master_write_slave(0, buffer, 2);

	for(n=0;n<128;n++)
        {
            buffer[0] = 0x40;
            buffer[1] = ram[m*128 + n];
            i2c_master_write_slave(0, buffer, 2);
        }
    }
}

int display_print(uint8_t *ram, uint8_t x, uint8_t y, char *text)
{
    for (int i = 0 ; i < strnlen(text, 21) ; i++)
    {
        for (int c = 0 ; c < 6 ; c++)
	{
            ram[y*128 + x+i*6 + c + 2] = fonts[(text[i] - 0x20)*6 + c];
	}
    }
    return 0;
}

static void *display_thread(void * arg)
{
	ESP_LOGI(TAG, "Thread started!");
    while (true)
    {
        sleep(1); 
    }
	ESP_LOGE(TAG, "Thread ended!");

    return NULL;
}

void display_init(void)
{
    pthread_attr_t attr;
    pthread_t threadDisplay;

    uint8_t buffer[2];
    int ret;

    memset(display_ram, 0, sizeof(display_ram));

    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    i2c_param_config(i2c_master_port, &conf);
  
    vTaskDelay(10);

    uint8_t init_cmds[] = {0xAE, 0x20, 0x10, 0xB0, 0xC8, 0x00, 0x10, 0x40, 0x81, 0xDF, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5, 0xF0, 0xD9, 0x22, 0xDA, 0x12, 0xDB, 0x20, 0x8D, 0x20, 0x8D, 0x14, 0xAF};

    buffer[0] = 0;
    for (int i = 0; i < sizeof(init_cmds) ; i++)
    {
        buffer[1] = init_cmds[i];
        i2c_master_write_slave(0, buffer, 2);
    }

    display_print(&display_ram[0][30], 0, 0, "www.ioko.eu");
    display_update(&display_ram[0][0]);

    // Start display thread
    ret = pthread_create(&threadDisplay, NULL, display_thread, NULL);
    assert(ret == 0);


    ret = pthread_attr_init(&attr);
    assert(ret == 0);
    pthread_attr_setstacksize(&attr, 10240);
    ret = pthread_create(&threadDisplay, &attr, display_thread, NULL);
    assert(ret == 0);
}