#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "user_interface.h"
#include "pwm.h"
#include "spi_flash.h"
#include "i2c_master.h"
#define DS3231_ADDRESS  0x68
uint32 io_info[][3] = {   {PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2,2},
                          };
u32 duty[1] = {0};

int i = 0;
int n = 0;
int sign  = 1;
int last_value = 0;
char BufferLia[128];

static volatile os_timer_t some_timer;

/******************************************************************************
 * FunctionName : save_variabel
 * Description  : save value into the flash memory ( EEPROM on Arduino)
 * Parameters   : address memory (0 - 63)
 *                int value (0 - 255)
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
save_variabel(int address,int value)
{
    char Dum_buff[64] = {0};
    int result = -1;
    result = spi_flash_read(0x3D000, (uint32 *)Dum_buff, 64);
    spi_flash_erase_sector(0x3D);
    char buff[64] = {0};
    int l = 0;
     for ( l = 0; l < 64; l++)
    {
        buff[l] = Dum_buff[l];
    }
    buff[address] = (char)value;
    result = spi_flash_write(0x3D000, (uint32 *)buff, 64);
}
/******************************************************************************
 * FunctionName : control_lamp
 * Description  : Control Lamp by Intensity
 * Parameters   : integer ( 0 - 100 )
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
control_lamp(int value) 
{
    if (value>100)value = 100;
    else if(value < 0)value = 0;

  if (value>last_value)
  {
    while(value>last_value)
    {
      last_value++;
      pwm_set_duty(last_value * 222, 0);  
      pwm_start();
      os_delay_us(10000);
    }
    save_variabel(1,value);
  }
  else if (value<last_value)
  {
    while(value<last_value)
    {
      last_value--;
      pwm_set_duty(last_value * 222, 0);  
      pwm_start();
      os_delay_us(10000);
    }
    save_variabel(1,value);
  }
}

/******************************************************************************
 * FunctionName : load_variabel
 * Description  : load value from the flash memory ( EEPROM on Arduino)
 * Parameters   : address memory (0 - 63)
 * Returns      : int value (0 - 255)
*******************************************************************************/
int ICACHE_FLASH_ATTR
load_variabel(int address)
{   
    char buff[64] = {0};
    int result = -1;
    int l = 0;
    result = spi_flash_read(0x3D000, (uint32 *)buff, 64);
    return (int)buff[address];
}

void some_timerfunc(void *arg)
{
    // control_lamp(i);
    // os_printf("adc = %d\n", system_adc_read());
    // os_printf("pwm_duty = %d\n", pwm_get_duty(0));
    // os_printf("pwm_periode = %d\n", pwm_get_period());
    // os_printf("RSSI = %d\n", wifi_station_get_rssi());
    // os_printf("mode Wifi = %d\n", wifi_get_phy_mode());
    // os_printf("EEPROM = %d\n\n", load_variabel(1));

    // if(sign) {
    //     sign = 0;
    //     i = 100;
    // }
    // else{
    //     sign = 1;
    //     i = 0;   

    i2c_master_start();
    i2c_master_writeByte(DS3231_ADDRESS);
    i2c_master_writeByte(0x30);
    i2c_master_getAck();
    i2c_master_writeByte(0x00);
    i2c_master_getAck();
    i2c_master_stop();

    i2c_master_start();
    i2c_master_writeByte(DS3231_ADDRESS);
    i2c_master_writeByte(0x31);
    i2c_master_getAck();
    os_printf("LIA : %d \n",i2c_master_readByte());
    i2c_master_send_nack();
    i2c_master_stop();
    

}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{   
    char ssid[32] = SSID;
    char password[64] = SSID_PASSWORD;
    struct station_config stationConf;

    //Set station mode
    wifi_set_opmode( 0x1 );

    //Set ap settings
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 64);
    wifi_station_set_config(&stationConf);

    pwm_init(1000, duty,1,io_info);
    i2c_master_gpio_init();
    
    i2c_master_start();
    i2c_master_writeByte(DS3231_ADDRESS);
    i2c_master_writeByte(0x30);
    i2c_master_getAck();
    i2c_master_writeByte(0x0E);
    i2c_master_getAck();
    i2c_master_writeByte(0x1C);
    i2c_master_getAck();
    i2c_master_stop(); 

    os_timer_disarm(&some_timer);
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
    os_timer_arm(&some_timer, 2000, 1);
    
}
