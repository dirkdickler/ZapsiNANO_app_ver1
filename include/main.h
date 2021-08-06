﻿#ifndef __MAIN_H
#define __MAIN_H

#include <Arduino.h>
#include "define.h"
#include <ESPAsyncWebServer.h>
#include "constants.h"

typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef const uint32_t uc32; /*!< Read Only */
typedef const uint16_t uc16; /*!< Read Only */
typedef const uint8_t uc8;	 /*!< Read Only */

typedef struct
{
	u8 Prea;
	u8 Fram;
	u8 DSTadress; //cilova adresa
	u8 SCRadress; //zdrojova adresa
	u8 SQCnum;
	u8 CMD;
	u8 MsgID;
	u16 Data_count; //7,8
	u8 sumaHead;
	u8 sumaData;
	u8 *data;
} AIR_PACKET_t;

typedef struct
{
	char name[16];
	i16 T_podlaha;
	i16 T_vzduch;
	i16 RH_vlhkkost;

	i16 T_podlaha_SET;
	i16 T_vzduch_SET;
	i16 RH_vlhkkost_SET;
	u8 teleso;
	u8 RS485_timeout;
} TERMOSTAT_t;

//definovani ID  sprav
#define MsgID_Ping 1
#define MsgID_BusMute 2
#define MsgID_TermostatData 3
#define MsgID_NameraneHodnoty 4
#define MsgID_Digital_vstupy 10
#define MsgID_Digital_vystupy 11

#define MsgID_PublicNazvyMistnosti 0xff00
#define MsgID_PublicHodnoty 0xff01
#define MsgID_GoToBoot 0xff02
#define MsgID_WriteToFlashAdress 0xff03
#define MsgID_ExitFromBoot 0xff04
#define MsgID_EraseAplSection 0xff05

//adrese v sieti
#define broadcast 0xFFFF

#define Preamble1 0x02
#define Frame 0x43

#define CMD_bit_RW 7
#define CMD_read 0
#define CMD_write 0x80

#define CMD_Ack_OK 0
#define CMD_Ack_Busy 1
#define CMD_Ack_NOK 2
#define CMD_Ack_Res1 4
#define CMD_Ack_Res2 8
#define CMD_Ack_Res3 0x10 //16
#define CMD_Reply 0x20	  //32
#define CMD_Event 0x40	  //64
#define CMD_RW 0x80		  //128
//ACK_NOK
#define NOKcode_countOverflow 1 //vela dat v pakete
#define NOKcode_sumaError 2		//nesedi suma
#define NOKcode_missingData 3	//chynaju data v pakete
#define NOKcode_naznamyMSGID 4	//

extern VSTUP_t DIN[pocetDIN_celkovo];
extern char NazovSiete[30];
extern char Heslo[30];
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;
extern IPAddress secondaryDNS;

void Loop_1ms(void);
void Loop_10ms(void);
void Loop_100ms(void);
void Loop_1sek(void);
void Loop_10sek(void);
void OdosliCasDoWS(void);
void DebugMsgToWebSocket(String textik);
void FuncServer_On(void);

void ESPinfo(void);
int getIpBlock(int index, String str);
String ipToString(IPAddress ip);
IPAddress str2IP(String str);
void OdosliStrankeVytapeniData(void);
String handle_Zadavanie_IP_setting(void);
void handle_Nastaveni(AsyncWebServerRequest *request);
void encoder();

#endif