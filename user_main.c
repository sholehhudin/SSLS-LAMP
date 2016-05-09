/* Lamp Controller
 * s.alayubi
 * sholehhudin.alayubi@gmail.com
 * last edited : 9 Mei 16
 * Thanks to :
            Richard A Burton (richardaburton@gmail.com). I2C driver for DS3231 RTC for ESP8266.
*/
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "user_interface.h"
#include "pwm.h"
#include "spi_flash.h"
#include "i2c_master.h"
#define DS3231_ADDRESS  0x68
#define DS3231_ADDR_TIME    0x00
#define DS3231_ADDR_TEMP    0x11
uint32 io_info[][3] = {{PERIPHS_IO_MUX_GPIO2_U,FUNC_GPIO2,2}};
u32 duty[1] = {0};

int i = 0;
int n = 0;
int sign  = 1;
int last_value = 0;
uint8 data[7];
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


/******************************************************************************
 * FunctionName : decToBcd
 * Description  : Convert Decimal value to BCD
 * Parameters   : Decimal number
 * Returns      : BCD number
*******************************************************************************/
static uint8 ICACHE_FLASH_ATTR decToBcd(uint8 dec) {
  return(((dec / 10) * 16) + (dec % 10));
}

/******************************************************************************
 * FunctionName : bcdToDec
 * Description  : Convert BCD value to Decimal
 * Parameters   : BCD number
 * Returns      : Decimal number
*******************************************************************************/
static uint8 ICACHE_FLASH_ATTR bcdToDec(uint8 bcd) {
  return(((bcd / 16) * 10) + (bcd % 16));
}

/******************************************************************************
 * FunctionName : ds3231_recv
 * Description  : read a number of bytes from the rtc over i2c
 * Parameters   : data and length of data
 * Returns      : bool
*******************************************************************************/
static bool ICACHE_FLASH_ATTR ds3231_recv(uint8 *data, uint8 len) {
    
    int loop;

    // signal i2c start
    i2c_master_start();

    // write address & direction
    i2c_master_writeByte((uint8)((DS3231_ADDRESS << 1) | 1));
    if (!i2c_master_checkAck()) {
        //uart0_send("i2c error\r\n");
        i2c_master_stop();
        return false;
    }

    // read bytes
    for (loop = 0; loop < len; loop++) {
        data[loop] = i2c_master_readByte();
        // send ack (except after last byte, then we send nack)
        if (loop < (len - 1)) i2c_master_send_ack(); else i2c_master_send_nack();
    }

    // signal i2c stop
    i2c_master_stop();

    return true;

}

/******************************************************************************
 * FunctionName : ds3231_send
 * Description  : send a number of bytes to the rtc over i2c
 * Parameters   : data and length of data
 * Returns      : bool
*******************************************************************************/
static bool ICACHE_FLASH_ATTR ds3231_send(uint8 *data, uint8 len) {
    
    int loop;

    // signal i2c start
    i2c_master_start();

    // write address & direction
    i2c_master_writeByte((uint8)(DS3231_ADDRESS << 1));
    if (!i2c_master_checkAck()) {
        //uart0_send("i2c error\r\n");
        i2c_master_stop();
        return false;
    }

    // send the data
    for (loop = 0; loop < len; loop++) {
        i2c_master_writeByte(data[loop]);
        if (!i2c_master_checkAck()) {
            //uart0_send("i2c error\r\n");
            i2c_master_stop();
            return false;
        }
    }

    // signal i2c stop
    i2c_master_stop();

    return true;

}

/******************************************************************************
 * FunctionName : RTC_get_sec
 * Description  : Get second of RTC times
 * Parameters   : none
 * Returns      : int8 second
*******************************************************************************/
static uint8 ICACHE_FLASH_ATTR RTC_get_sec() {
    // start register address
    data[0] = DS3231_ADDR_TIME;
    if (!ds3231_send(data, 1)) {
        os_printf("gagal brooh1\n");
    }

    // read time
    if (!ds3231_recv(data, 7)) {
        os_printf("gagal brooh2\n");
    }
  return bcdToDec(data[0]);
}

/******************************************************************************
 * FunctionName : RTC_get_min
 * Description  : Get minute of RTC times
 * Parameters   : none
 * Returns      : int8 minute
*******************************************************************************/
static uint8 ICACHE_FLASH_ATTR RTC_get_min() {
    // start register address
    data[0] = DS3231_ADDR_TIME;
    if (!ds3231_send(data, 1)) {
        os_printf("gagal brooh1\n");
    }

    // read time
    if (!ds3231_recv(data, 7)) {
        os_printf("gagal brooh2\n");
    }
  return bcdToDec(data[1]);
}

/******************************************************************************
 * FunctionName : RTC_get_hour
 * Description  : Get hour of RTC times
 * Parameters   : none
 * Returns      : int8 hour
*******************************************************************************/
static uint8 ICACHE_FLASH_ATTR RTC_get_hour() {
    // start register address
    data[0] = DS3231_ADDR_TIME;
    if (!ds3231_send(data, 1)) {
        os_printf("gagal brooh1\n");
    }

    // read time
    if (!ds3231_recv(data, 7)) {
        os_printf("gagal brooh2\n");
    }
  return bcdToDec(data[2]);
}

/******************************************************************************
 * FunctionName : ds3231_getTempInteger
 * Description  : get tempt of ds3231
 * Parameters   : none
 * Returns      : int8 celcius
*******************************************************************************/
int8 ICACHE_FLASH_ATTR ds3231_getTempInteger() {
    uint8 data1[1];
    int8 Nilai = 0;
    data1[0] = DS3231_ADDR_TEMP;
    if (ds3231_send(data1, 1) && ds3231_recv(data1, 1)) {
        Nilai = (signed)data1[0];
    }
    return Nilai;
}

/******************************************************************************
 * FunctionName : ds3231_setTime
 * Description  : set time of ds3231
 * Parameters   : 
                    year     -> year
                    month    -> month
                    day      -> day
                    hour     -> hour
                    min      -> minute
                    sec      -> second
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR ds3231_setTime(int year,uint8 mon,uint8 day,uint8 hour,uint8 min,uint8 sec) {
    
    uint8 data[8];

    // start register
    data[0] = DS3231_ADDR_TIME;
    // time/date data
    data[1] = decToBcd(sec);
    data[2] = decToBcd(min);
    data[3] = decToBcd(hour);
    data[4] = decToBcd(0 + 1);
    data[5] = decToBcd(day);
    data[6] = decToBcd(mon + 1);
    data[7] = decToBcd(year - 100);

    ds3231_send(data, 8);

}

void ICACHE_FLASH_ATTR
send_temp()
{
    int This_variabel_is_temp = ds3231_getTempInteger();
    os_printf("temp = %d C\n", This_variabel_is_temp);
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
  int8 _minute = RTC_get_min();
  int8 _hour = RTC_get_hour();


    if (6*60+00 > _minute + _hour*60 && _minute + _hour*60 >=0)
    {
      // set dimmer of lamp to 50 % from 00.00 to 05.59
      control_lamp(50);
    }
    else if (18*60+00 > _minute + _hour*60 && _minute + _hour*60 >=6*60+00)
    {
      // set dimmer of lamp to 0 % from 06.00 to 17.59
      control_lamp(0);
    }
    else if (24*60+00 > _minute + _hour*60 && _minute + _hour*60 >=18*60+00)
    {
      // set dimmer of lamp to 100 % from 18.00 to 23.59
      control_lamp(100);
    }
}
void some_timerfunc(void *arg)
{
    /*
        device will check status connection. and if connection not reliable. device will activate Local Inteligent
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
        send_temp();
    }
    os_printf("Menit = %d\n", RTC_get_min());

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

/*
  interrupt handler
  i hope you can make interupt handler base kaa to control lamp.(like subscribe on MQTT)
  
  some task need update "value" from interupt handler like :
  1. control_lamp(value).
    for control dimmer of lamp
    need : 1 value (integer)

  2. ds3231_setTime(a,b,c,d,e,f)
    to set time RTC
    need : 6 value (integer) json is OK.
*/
