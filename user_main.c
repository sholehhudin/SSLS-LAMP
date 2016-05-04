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
bool Connet_To_sever = false;

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
 * FunctionName : send_RSSI
 * Description  : send stat RSSI to server
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
send_RSSI()
{
    int This_variabel_is_RSSI = wifi_station_get_rssi();
    os_printf("RSSI = %d\n", This_variabel_is_RSSI);
    /*
    please add your kaa procedure to send this data to server at here.
    */
}

/******************************************************************************
 * FunctionName : send_current
 * Description  : send value of current to server
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
send_current()
{
    int This_variabel_is_current = system_adc_read();
    os_printf("current = %d\n", This_variabel_is_current);
    /*
    please add your kaa procedure to send this data to server at here.
    */
}

/******************************************************************************
 * FunctionName : inteligent_local
 * Description  : control dimmer lamp, if connection is not reliable
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
inteligent_local()
{
    // still developing
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
    /*
        > device will check every one minute to get new value of Dimmer
        > i hope you can make interupt handler base kaa to control lamp.(like subscribe on MQTT)
    */
       control_lamp(i);



    /*
        device will check status connection. and if connection not reliable. device will activate Local Inteliget
        local inteligent will handle dimmer of lamp base RTC timer.
    */
        if (Connet_To_sever)
        {
            inteligent_local();
        }

    


     //   device will send some parameter to kaa server every 10 minutes. like RSSI, tempt, and current sensor.
    if (n>=10)
    {
        n = 0;
        send_RSSI();
        send_current();


    }

}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{   
    control_lamp(load_variabel(1)); 
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

    //timer 60 s
    os_timer_disarm(&some_timer);
    os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
    os_timer_arm(&some_timer, 60000, 1);
    
}
