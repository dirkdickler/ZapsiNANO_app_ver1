#ifndef __DEFINE_H
#define __DEFINE_H

#define  pocetHTTPspojeni 2
#define  TX_RX_MAX_BUF_SIZE 4096
#define TCP_10001_socket 3

#define input1 0
#define input2 1
#define input3 2
#define input4 3
#define input_SDkarta 5 

#define pocetDIN 4
#define pocetDIN_celkovo 6   //DIN + SD CD pin , !!! ale daj o +1 lebo nevim preco ho to prepisovalo davalo do neho RTC sek :-) 
#define filterTime_DI 3  //10ms loop
#define filterTime_SD_CD 10 //10ms loop


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