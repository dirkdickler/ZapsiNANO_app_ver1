#ifndef __DEFINE_H
#define __DEFINE_H

typedef struct
{
	String Nazov;
	float Tset;
}ROOM_t;

#define input1 0
#define input2 1
#define input3 2
#define input4 3

#define pocetDIN 4
#define filterTime_DI 3


//const int LedGreen = 26;
#define ADC_gain_pin  1
#define ADC_curren_pin  2

#define DI4_pin 3
#define DI3_pin 4
#define DI2_pin 5
#define DI1_pin 7
#define SD_miso 11
#define SD_mosi 12
#define SD_sck 10
#define SD_CS_pin 8
#define SD_CD_pin 7
#define WIZ_CS_pin 9
#define WIZ_INT_pin 13
#define WIZ_RES_pin 14
#define SDA_pin 17
#define SCL_pin 18
#define Joy_up_pin 36
#define Joy_dn_pin 35
#define Joy_Butt_pin 34
#define Joy_left_pin 33
#define Joy_right_pin 21


#define WDT_TIMEOUT 5
#define firmware "ver20210614_1beta"


//EEPROM adrese
#define EE_IPadresa 00				//16bytes
#define EE_SUBNET 16				//16bytes
#define EE_Brana 32					//16bytes
#define EE_NazovSiete 48			//16bytes
#define EE_Heslosiete 64			//20bytes
#define EE_citacZapisuDoEEPORM 84   //2bytes
#define EE_citac2_ZapisuDoEEPORM 86   //2bytes
#define EE_dalsi 88
#define EE_zacateKaret_1 200
#define EE_zacateKaret_2 1300   //EE_zacateKaret + 100*11tj 1300


#endif 