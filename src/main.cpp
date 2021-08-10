
// Import required libraries
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <esp_task_wdt.h>
#include "time.h"
#include <Ticker.h>
#include <EEPROM.h>
#include "index.html"
#include "main.h"
#include "define.h"
#include "constants.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SoftwareSerial.h>
#include <ESP32Time.h>
#include "HelpFunction.h"
#include "Pin_assigment.h"
#include "WizChip_my_API.h"

#define ENCODER1 2
#define ENCODER2 3
volatile long int encoder_pos = 0;
int AN_Pot1_Raw = 0;
int AN_Pot1_Filtered = 0;

// Replace with your network credentials
//const char* ssid = "Grabcovi";
//const char* password = "40177298";
const char *soft_ap_ssid = "aDum_Server";
const char *soft_ap_password = "aaaaaaaaaa";
//const char *ssid = "semiart";
//const char *password = "aabbccddff";
char NazovSiete[30];
char Heslo[30];

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar myObject, myObject2, ObjDatumCas, ObjTopeni, JSON_DebugMsg;
Ticker timer_1ms(Loop_1ms, 1, 0, MILLIS);
Ticker timer_10ms(Loop_10ms, 10, 0, MILLIS);
Ticker timer_100ms(Loop_100ms, 300, 0, MILLIS);
Ticker timer_1sek(Loop_1sek, 1000, 0, MILLIS);
Ticker timer_10sek(Loop_10sek, 10000, 0, MILLIS);

SoftwareSerial swSer(14, 12, false, 256);
char swTxBuffer[16];
char swRxBuffer[16];

SPIClass SDSPI(HSPI);

ESP32Time rtc;

IPAddress local_IP(192, 168, 1, 14);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);	//optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600; //letny cas
struct tm MyRTC_cas;
bool Internet_CasDostupny = false; //to je ze dostava cas z Inernetu
bool RTC_cas_OK = false;			  //ze mam RTC fakt nastaveny bud z interneru, alebo nastaveny manualne
										  //a to teda ze v RTC mam fakr realny cas
										  //Tento FLAG, nastavi len pri nacitanie casu z internutu, alebo do buducna manualne nastavenie casu cew WEB

const u8 mojePrmenaae = 25;
u16_t cnt = 0;

char gloBuff[200];
bool LogEnebleWebPage = false;

VSTUP_t DIN[pocetDIN_celkovo];
char TX_BUF[TX_RX_MAX_BUF_SIZE];
//------------------------------------------------------------------------------------------------------------------
wiz_NetInfo eth =
	{
		.mac = {0x80, 0x1F, 0x12, 0x56, 0xC7, 0xC9},
		.ip = {192, 168, 1, 10},
		.sn = {255, 255, 255, 0},
		.gw = {192, 168, 1, 1},
		.dns = {8, 8, 8, 8},
		.dhcp = NETINFO_DHCP};

/**********************************************************
 ***************        SETUP         **************
 **********************************************************/

void setup()
{
	Serial.begin(115200);
	Serial.println("Spustam applikaciu.ab..");
	System_init();

	//attachInterrupt(digitalPinToInterrupt(ENCODER1), encoder, RISING);
	//pinMode(ENCODER1, INPUT);
	//pinMode(ENCODER2, INPUT);

	ESPinfo();

	myObject["hello"] = " 11:22 Streda";
	myObject["true"] = true;
	myObject["x"] = 42;
	myObject2["Citac"] = 42;

	NacitajEEPROM_setting();

	//WiFi_init();    //este si odkomentuj  //WiFi_connect_sequencer(); v 10 sek loop
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	timer_1ms.start();
	timer_10ms.start();
	timer_100ms.start();
	timer_1sek.start();
	timer_10sek.start();
	esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
	esp_task_wdt_add(NULL);				  //add current thread to WDT watch

	//RS485 musis spustit az tu, lebo ak ju das hore a ESP ceka na konnect wifi, a pridu nejake data na RS485, tak FreeRTOS =RESET  asi overflow;
	//Serial1.begin(9600);

	//swSer.begin(115200);
	//swSer.println("");
}

void loop()
{
	esp_task_wdt_reset();
	ws.cleanupClients();
	//AsyncElegantOTA.loop();
	timer_1ms.update();
	timer_10ms.update();
	timer_100ms.update();
	timer_1sek.update();
	timer_10sek.update();
}

void Loop_1ms()
{
	AN_Pot1_Raw = analogRead(ADC_curren_pin);
	AN_Pot1_Filtered = readADC_Avg(AN_Pot1_Raw);

	TCP_handler(TCP_10001_socket, 10001);
}

void Loop_10ms()
{
	static uint8_t TimeOut_RXdata = 0;	 //musi byt static lebo sem skaces z Loop
	static uint16_t KolkkoNplnenych = 0; //musi byt static lebo sem skaces z Loop
	static char budd[250];				 //musi byt static lebo sem skaces z Loop

	uint16_t aktualny;
	char temp[200];

	aktualny = Serial1.available();
	if (aktualny)
	{

		//Serial2.readBytes (temp, aktualny);
		for (uint16_t i = 0; i < aktualny; i++)
		{
			if ((KolkkoNplnenych + aktualny) < sizeof(budd))
			{
				budd[KolkkoNplnenych + i] = Serial1.read();
			}
			else
			{
				Serial1.read();
			}
		}
		KolkkoNplnenych += aktualny;
		TimeOut_RXdata = 5;
	}

	if (TimeOut_RXdata)
	{
		if (--TimeOut_RXdata == 0)
		{
			{
				sprintf(temp, "[RS485] doslo:%u a to %s\r\n", KolkkoNplnenych, budd);
				//DebugMsgToWebSocket(temp);
				//Serial.printf(temp);

				AIR_PACKET_t *loc_paket;
				loc_paket = (AIR_PACKET_t *)budd;

				sprintf(temp, "[RS485]  DST adresa je:%u\r\n", loc_paket->DSTadress);
				//DebugMsgToWebSocket(temp);
				//Serial.printf(temp);

				sprintf(temp, "[RS485] Mam adresu %u a idem ulozit data z RS485\r\n", loc_paket->SCRadress);
				//DebugMsgToWebSocket(temp);

				if (loc_paket->SCRadress == 10)
				{
					//	OdosliStrankeVytapeniData();
				}

				memset(budd, 0, sizeof(budd));
				KolkkoNplnenych = 0;
			}
		}
	}

	ScanInputs();
}

void Loop_100ms(void)
{
}

void Loop_1sek(void)
{
	Serial.print("[1sek Loop]  mam 1 sek....  ");
	String sprava = rtc.getTime("[%H:%M:%S] karta ");
	if (digitalRead(SD_CD_pin) == LOW)
	{
		//sprintf(TX_BUF, "[1sek Loop]  karta zasunota\r\n");
		sprava += "zasunota\r\n";
	}
	else
	{
		//sprintf(TX_BUF, "[1sek Loop]  karta Vysunuta\r\n");
		sprava += "Vysunuta\r\n";
	}
	sprava.toCharArray(TX_BUF, TX_RX_MAX_BUF_SIZE, 0);
	send(TCP_10001_socket, (u8 *)TX_BUF, strlen(TX_BUF));

	if (Internet_CasDostupny == false)
	{
		Serial.print("Internet cas nedostupny !!,  ");
	}
	else
	{
		Serial.print("Internet cas dostupny,  ");
	}
	Serial.print("RTC cas cez func rtc.getTime: ");
	Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
	MyRTC_cas = rtc.getTimeStruct();
	//Serial.print("[1sek Loop]  free Heap je:");
	//Serial.println(ESP.getFreeHeap());
}

void Loop_10sek(void)
{
	static u8_t loc_cnt_10sek = 0;
	Serial.println("\r\n[10sek Loop]  Mam Loop 10 sek..........");
	//DebugMsgToWebSocket("[10sek Loop]  mam 10 sek....\r\n");

	//TODOtu si teraz spra ukaldanie analogu a digital cnt na do RAM or do SD karty
	//{

	//tu pred touto loop musis mat uz ulozene DIN.counters, lebo si ich tu nulujem!!
	//for (u8 i = 0; i < pocetDIN; i++)
	//{
	//		DIN[pocetDIN].counter = 0;
	//	}
	//}

	//WiFi_connect_sequencer();
}

void OdosliCasDoWS(void)
{
	String DenvTyzdni = "! Čas nedostupný !";
	char loc_buf[60];
	DenvTyzdni = ConvetWeekDay_UStoSK(&MyRTC_cas);
	char hodiny[5];
	char minuty[5];
	strftime(loc_buf, sizeof(loc_buf), " %H:%M    %d.%m.%Y    ", &MyRTC_cas);

	ObjDatumCas["Cas"] = loc_buf + DenvTyzdni; // " 11:22 Stredaaaa";
	String jsonString = JSON.stringify(ObjDatumCas);
	Serial.print("[10sek] Odosielam strankam ws Cas:");
	Serial.println(jsonString);
	ws.textAll(jsonString);
}

void DebugMsgToWebSocket(String textik)
{
	if (LogEnebleWebPage == true)
	{
		String sprava = rtc.getTime("%H:%M:%S ");
		JSON_DebugMsg["DebugMsg"] = sprava + textik;
		String jsonString = JSON.stringify(JSON_DebugMsg);
		ws.textAll(jsonString);
	}
}

void FuncServer_On(void)
{
	server.on("/",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  //if (!request->authenticate("ahoj", "xxxx"))
				  //return request->requestAuthentication();
				  //request->send_P(200, "text/html", index_html, processor);
				  request->send_P(200, "text/html", Main);
			  });

	server.on("/nastavip",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  if (!request->authenticate("admin", "adum"))
					  return request->requestAuthentication();
				  request->send(200, "text/html", handle_Zadavanie_IP_setting());
			  });

	server.on("/Nastaveni",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  handle_Nastaveni(request);
				  request->send(200, "text/html", "Nastavujem a ukladam do EEPROM");
				  Serial.println("Idem resetovat ESP");
				  delay(2000);
				  esp_restart();
			  });

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
			  {
				  char ttt[500];
				  //u16_t citac = EEPROM.readUShort (EE_citacZapisuDoEEPORM);
				  //u16_t citac2 = EEPROM.readUShort (EE_citac2_ZapisuDoEEPORM);

				  char loc_buf[20];
				  char loc_buf1[60];
				  char loc_buf2[100];
				  if (Internet_CasDostupny == true)
				  {
					  sprintf(loc_buf, "dostupny :-)");
				  }
				  else
				  {
					  sprintf(loc_buf, "nedostupny!!");
				  }

				  if (RTC_cas_OK == false)
				  {
					  sprintf(loc_buf2, "[RTC_cas_OK == flase] RTC NE-maju realny cas!!. RTC hodnota: ");
				  }
				  else
				  {
					  sprintf(loc_buf2, "[RTC_cas_OK == true] RTC hodnota: ");
				  }
				  strftime(loc_buf1, sizeof(loc_buf1), " %H:%M:%S    %d.%m.%Y    ", &MyRTC_cas);

				  sprintf(ttt, "Firmware :%s<br>"
							   "Sila signalu WIFI(-30 je akoze OK):%i<br>"
							   "Internet cas: %s<br>"
							   "%s %s",
						  firmware, WiFi.RSSI(), loc_buf, loc_buf2, loc_buf1);

				  request->send(200, "text/html", ttt);
			  });

	server.on("/reset",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  if (!request->authenticate("admin", "radecek78"))
					  return request->requestAuthentication();

				  request->send(200, "text/html", "resetujem!!!");
				  delay(1000);
				  esp_restart();
			  });

	server.on("/vytapeni",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  request->send_P(200, "text/html", vytapeni);
			  });

	server.on("/zaluzie_Main",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  request->send_P(200, "text/html", zaluzie_Main);
			  });
	server.on("/debug",
			  HTTP_GET,
			  [](AsyncWebServerRequest *request)
			  {
				  LogEnebleWebPage = true;
				  request->send_P(200, "text/html", DebugLog_html);
			  });
}

//***********************************************  Hepl function ********************************************/

void ESPinfo(void)
{
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	Serial.println("\r\n*******************************************************************");
	Serial.print("ESP Board MAC Address:  ");
	Serial.println(WiFi.macAddress());
	Serial.println("\r\nHardware info");
	Serial.printf("%d cores Wifi %s%s\n",
				  chip_info.cores,
				  (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
				  (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
	Serial.printf("\r\nSilicon revision: %d\r\n ", chip_info.revision);
	Serial.printf("%dMB %s flash\r\n",
				  spi_flash_get_chip_size() / (1024 * 1024),
				  (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embeded" : "external");

	Serial.printf("\r\nTotal heap: %d\r\n", ESP.getHeapSize());
	Serial.printf("Free heap: %d\r\n", ESP.getFreeHeap());
	Serial.printf("Total PSRAM: %d\r\n", ESP.getPsramSize());
	Serial.printf("Free PSRAM: %d\r\n", ESP.getFreePsram()); // log_d("Free PSRAM: %d", ESP.getFreePsram());
	Serial.print("Alokujem buffer psdRamBuffer  500kB PSRAM");
	byte *psdRamBuffer = (byte *)ps_malloc(500000);
	Serial.printf(" -Free PSRAM: %d\r\n", ESP.getFreePsram());
	Serial.print("Uvolnujem buffer psdRamBuffer 500kz PSRAM ");
	free(psdRamBuffer);
	Serial.printf(" Free PSRAM po uvolneni : %d\r\n", ESP.getFreePsram()); // log_d("Free PSRAM: %d", ESP.getFreePsram());
	Serial.println("\r\n*******************************************************************");
}

int getIpBlock(int index, String str)
{
	char separator = '.';
	int found = 0;
	int strIndex[] = {0, -1};
	int maxIndex = str.length() - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++)
	{
		if (str.charAt(i) == separator || i == maxIndex)
		{
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}

	return found > index ? str.substring(strIndex[0], strIndex[1]).toInt() : 0;
}

String ipToString(IPAddress ip)
{
	String s = "";
	for (int i = 0; i < 4; i++)
		s += i ? "." + String(ip[i]) : String(ip[i]);
	return s;
}

IPAddress str2IP(String str)
{

	IPAddress ret(getIpBlock(0, str), getIpBlock(1, str), getIpBlock(2, str), getIpBlock(3, str));
	return ret;
}

String handle_LenZobraz_IP_setting(void)
{
	char ttt[1000];
	char NazovSiete[56] = {"nazovSiete\0"};
	char ippadresa[56];
	char maskaIP[56];
	char brana[56];

	EEPROM.readString(EE_NazovSiete, NazovSiete, 16);
	IPAddress ip = WiFi.localIP();
	String stt = ipToString(ip);
	stt.toCharArray(ippadresa, 16);
	stt = ipToString(WiFi.subnetMask());
	stt.toCharArray(maskaIP, 16);
	stt = ipToString(WiFi.gatewayIP());
	stt.toCharArray(brana, 16);

	Serial.print("\r\nVyparsrovane IP: ");
	Serial.print(ippadresa);
	Serial.print("\r\nVyparsrovane MASKa: ");
	Serial.print(maskaIP);
	Serial.print("\r\nVyparsrovane Brana: ");
	Serial.print(brana);

	sprintf(ttt, LenzobrazIP_html, ippadresa, maskaIP, brana, NazovSiete);
	return ttt;
	//server.send (200, "text/html", ttt);
}

String handle_Zadavanie_IP_setting()
{
	char ttt[1000];
	char NazovSiete[56] = {"nazovSiete\0"};
	char HesloSiete[56] = {"HesloSiete\0"};
	char ippadresa[56];
	char maskaIP[56];
	char brana[56];

	IPAddress ip = WiFi.localIP();
	String stt = ipToString(ip);
	stt.toCharArray(ippadresa, 16);
	stt = ipToString(WiFi.subnetMask());
	stt.toCharArray(maskaIP, 16);
	stt = ipToString(WiFi.gatewayIP());
	stt.toCharArray(brana, 16);

	EEPROM.readString(EE_NazovSiete, NazovSiete, 16);
	Serial.print("\r\nNazov siete: ");
	Serial.print(NazovSiete);

	EEPROM.readString(EE_Heslosiete, HesloSiete, 16);
	Serial.print("\r\nHeslo siete: ");
	Serial.print(HesloSiete);

	Serial.print("\r\nVyparsrovane IP: ");
	Serial.print(ippadresa);
	Serial.print("\r\nVyparsrovane MASKa: ");
	Serial.print(maskaIP);
	Serial.print("\r\nVyparsrovane Brana: ");
	Serial.print(brana);

	sprintf(ttt, zadavaci_html, ippadresa, maskaIP, brana, NazovSiete, HesloSiete);
	//Serial.print ("\r\nToto je bufer pre stranky:\r\n");
	//Serial.print(ttt);

	return ttt;
}

void handle_Nastaveni(AsyncWebServerRequest *request)
{
	String inputMessage;
	String inputParam;
	Serial.println("Mam tu nastaveni");

	if (request->hasParam("input1"))
	{
		inputMessage = request->getParam("input1")->value();
		if (inputMessage.length() < 16)
		{
			EEPROM.writeString(EE_IPadresa, inputMessage);
		}
	}

	if (request->hasParam("input2"))
	{
		inputMessage = request->getParam("input2")->value();
		if (inputMessage.length() < 16)
		{
			EEPROM.writeString(EE_SUBNET, inputMessage);
		}
	}

	if (request->hasParam("input3"))
	{
		inputMessage = request->getParam("input3")->value();
		if (inputMessage.length() < 16)
		{
			EEPROM.writeString(EE_Brana, inputMessage);
		}
	}

	if (request->hasParam("input4"))
	{
		inputMessage = request->getParam("input4")->value();
		if (inputMessage.length() < 16)
		{
			EEPROM.writeString(EE_NazovSiete, inputMessage);
		}
	}

	if (request->hasParam("input5"))
	{
		inputMessage = request->getParam("input5")->value();
		if (inputMessage.length() < 20)
		{
			EEPROM.writeString(EE_Heslosiete, inputMessage);
		}
	}

	EEPROM.commit();
}

void OdosliStrankeVytapeniData(void)
{
	//ObjTopeni["tep1"] = room[0].T_vzduch;
	//ObjTopeni["hum1"] = room[0].RH_vlhkkost;

	String jsonString = JSON.stringify(ObjTopeni);
	Serial.print("[ event -VratNamerane_TaH] Odosielam strankam ObjTopeni:");
	//Serial.println(jsonString);
	ws.textAll(jsonString);
}

void encoder()
{

	if (digitalRead(ENCODER2) == HIGH)
	{
		encoder_pos++;
	}
	else
	{
		encoder_pos--;
	}
}

//******************************************************************************************************************************
void TCP_handler(uint8_t s, uint16_t port)
{

	uint16_t RSR_len;
	char *ethBuff = TX_BUF;
	uint8_t ret; /* if a socket is closed */
	int32_t kolko;
	uint16_t freesize;
	uint16_t size = 0, sentsize = 0;
	switch (getSn_SR(s))
	{
	case SOCK_ESTABLISHED: /* if connection is established */
		if (getSn_IR(s) & Sn_IR_CON)
		{
			setSn_IR(s, Sn_IR_CON);
		}
		if ((size = getSn_RX_RSR(s)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
		{
			if (size > TX_RX_MAX_BUF_SIZE)
				size = TX_RX_MAX_BUF_SIZE;

			ret = recv(s, (u8 *)ethBuff, size);

			if (ret <= 0)
				return; // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
			size = (uint16_t)ret;
			sentsize = 0;

			if (strncmp((const char *)ethBuff, "GET", 3) == 0) // && timers.GET_request_timeout == 0 )
			{
				//SDSPI.setFrequency(35000000);

				sprintf(TX_BUF, "\r\n*****DOSLO GET!!!!");
				unsigned long start = micros();
				send(s, (u8 *)ethBuff, strlen((const char *)ethBuff));
				unsigned long end = micros();
				unsigned long delta = end - start;
				Serial.print("DELTA: ");
				Serial.println(delta);
			}
			sprintf(TX_BUF, "\r\n*****Test ci ospovida Wiz5100s!");
			send(s, (u8 *)ethBuff, strlen(ethBuff));
		}
		break;

	case SOCK_CLOSE_WAIT: /* If the client request to close */

		disconnect(s);
		break;

	case SOCK_CLOSED:
		if ((ret = socket(s, Sn_MR_TCP, port, 0x00)) != s) //if(socket(s,Sn_MR_TCP,port,0x00) == 0)    /* reinitialize the socket */
		{
		}
		break;

	case SOCK_INIT: /* if a socket is initiated */
		listen(s);
		break;

	default:
		break;
	}
}
//******************************************************************************************************************************
