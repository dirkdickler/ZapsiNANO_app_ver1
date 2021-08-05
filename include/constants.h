#ifndef INC_CONSTANST_H
#define INC_CONSTANST_H

#include <Arduino.h>
#include "define.h"
#include <ESPAsyncWebServer.h>



static const char deviceName[31] = { "Zapsi Eng FW 1                " };
static const char MCU_type[9] = { "F412RE" };

static const char path_DI_Lock_txt[]  = { "log/di_lock.txt" };
static const char path_digital_txt[]  = { "log/digital.txt" };
static const char path_Digital_txt[]  = { "LOG/digital.txt" };
static const char path_analog_txt[]   = { "log/analog.txt" };
static const char path_Analog_txt[]   = { "LOG/analog.txt" };
static const char path_rs232_txt[]    = { "log/rs232.txt" };
static const char path_power_txt[]    = { "log/UI_value.txt" };
static const char path_Power_txt[]    = { "Log/UI_value.txt" };



typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef const int32_t sc32; /*!< Read Only */
typedef const int16_t sc16; /*!< Read Only */
typedef const int8_t sc8;   /*!< Read Only */

typedef volatile int32_t vs32;
typedef volatile int16_t vs16;
typedef volatile int8_t vs8;

typedef volatile const int32_t vsc32; /*!< Read Only */
typedef volatile const int16_t vsc16; /*!< Read Only */
typedef volatile const int8_t vsc8;   /*!< Read Only */

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef const uint32_t uc32; /*!< Read Only */
typedef const uint16_t uc16; /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */

typedef volatile uint32_t vu32;
typedef volatile uint16_t vu16;
typedef volatile uint8_t vu8;

typedef volatile const uint32_t vuc32; /*!< Read Only */
typedef volatile const uint16_t vuc16; /*!< Read Only */
typedef volatile const uint8_t vuc8;   /*!< Read Only */

//definovani ID  sprav
#define MsgID_Ping            1 
#define MsgID_AllDataSpace    2 
#define MsgID_SingleNode      3
#define MsgID_NameraneHodnoty 4

#define MsgID_PublicNazvyMistnosti  0xff00 
#define MsgID_PublicHodnoty 0xff01
#define MsgID_GoToBoot 0xff02
#define MsgID_WriteToFlashAdress 0xff03
#define MsgID_ExitFromBoot       0xff04


#define  broadcast 0xFFFF

#define  Preamble1 0x02
#define  Frame     0x43

#define  CMD_bit_RW 7  
#define  CMD_read  0 
#define  CMD_write 0x80 


#define CMD_Ack_OK    0
#define CMD_Ack_Busy  1
#define CMD_Ack_NOK   2
#define CMD_Reply     0x20
#define CMD_Event     0x40
#define CMD_RW    0x80

//ACK_NOK 
#define NOKcode_countOverflow  1  //vela dat v pakete
#define NOKcode_sumaError      2  //nesedi suma
#define NOKcode_missingData    3  //chynaju data v pakete
#define NOKcode_naznamyMSGID   4  //


typedef struct
{  
	u8 typVstupu;     //klasika,citac,ci ma automaticky padat fo nule
	uint16_t pin;
	uint8_t input;
	uint8_t input_prew;
	uint16_t filter;
	uint32_t counter;
	uint16_t conter_filter;
	bool zmena;
}VSTUP_t;


typedef struct //
{
	bool   RTCinit_done;
	bool   eth_connect;
	bool   test_spustDNS;
	bool   SDkarta_OK;
	bool   analog10sek_uz_ulozoeny;
	bool   minutaPerioda_uz_spracovana;

} FLAGS_t;

typedef struct //
{
	u8   RAM_DISK_analog_lock;
	u8   RAM_DISK_Input_lock;
	u8   RAM_DISK_power_lock;
} SEMAFOR_t;


typedef struct //
{
	uint16_t hodina;
	uint8_t OutPut_DisableTimeout_sec;
	uint8_t sekunda;
	uint16_t GET_request_timeout;
} TIMERS_t;

typedef struct //
{
	uint8_t byte;
} DigitalOUT_t;

extern SEMAFOR_t semafor;
extern FLAGS_t flg;
extern TIMERS_t timer;
extern DigitalOUT_t DO[];

#endif
