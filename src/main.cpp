
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
#include <TimeLib.h>
#include <Wire.h>
#include "pcf8563.h"
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
#include "Middleware\Ethernet\WizChip_my_API.h"
#include "esp_log.h"

//#include "sdusb.h"
//#include "mscusb.h"

#define ENCODER1 2
#define ENCODER2 3
volatile long int encoder_pos = 0;

// Replace with your network credentials
// const char* ssid = "Grabcovi";
// const char* password = "40177298";
const char *soft_ap_ssid = "aDum_Server";
const char *soft_ap_password = "aaaaaaaaaa";
// const char *ssid = "semiart";
// const char *password = "aabbccddff";
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
PCF8563_Class PCFrtc;
IPAddress local_IP(192, 168, 1, 14);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);	// optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;		 // 3600;
const int daylightOffset_sec = 0; // 3600; //letny cas
struct tm MyRTC_cas;
bool Internet_CasDostupny = false; // to je ze dostava cas z Inernetu
bool RTC_cas_OK = false;			  // ze mam RTC fakt nastaveny bud z interneru, alebo nastaveny manualne
											  // a to teda ze v RTC mam fakr realny cas
											  // Tento FLAG, nastavi len pri nacitanie casu z internutu, alebo do buducna manualne nastavenie casu cew WEB

u16_t SCT_prud_0 = 0;

char gloBuff[200];
bool LogEnebleWebPage = false;

FLAGS_t flg;
LOGBUFF_t LogBuffer;

#include "sdusb.h"
SDCard2USB dev;

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

#define SD_MISO 37
#define SD_MOSI 39
#define SD_SCK 38
#define SD_CS 40
void setup()
{
	Serial.begin(115200);
	Serial.println("Spustam applikaciu.a1");
	System_init();

	ESP_LOGW("", "est ESLP log W");
	ESP_LOGI("TEST SP log I", "storage usedd: %lld/%lld", 23, 24);
	// attachInterrupt(digitalPinToInterrupt(ENCODER1), encoder, RISING);
	// pinMode(ENCODER1, INPUT);
	// pinMode(ENCODER2, INPUT);

	if (dev.initSD(SD_SCK, SD_MISO, SD_MOSI, SD_CS))
	{
		if (dev.begin())
		{
			Serial.println("MSC lun 1 begin");
		}
		else
			log_e("LUN 1 failed");
	}
	else
		Serial.println("Failed to init SD");

	ESPinfo();

	myObject["Parameter"]["gateway"]["value"] = " 11:22 Strezda";
	myObject["tes"]["ddd"] = 42;
	myObject["hello"] = "11:22 Streda";
	myObject["true"] = true;
	myObject["x"] = 42;
	myObject2["Citac"] = "ahojj";

	String jsonString = JSON.stringify(myObject);
	Serial.print("JSON strinf dump");
	Serial.println(jsonString);

	Serial.print("myObject.keys() = ");
	Serial.println(myObject.keys());

	myObject2["Citac"] = myObject["Parameter"]["gateway"]["value"];
	jsonString = JSON.stringify(myObject2);
	Serial.print("JSON2 strinf dump");
	Serial.println(jsonString);

	NacitajEEPROM_setting();

	// WiFi_init();    //este si odkomentuj  //WiFi_connect_sequencer(); v 10 sek loop
	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	timer_1ms.start();
	timer_10ms.start();
	timer_100ms.start();
	timer_1sek.start();
	timer_10sek.start();
	esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
	esp_task_wdt_add(NULL);					  // add current thread to WDT watch

	// RS485 musis spustit az tu, lebo ak ju das hore a ESP ceka na konnect wifi, a pridu nejake data na RS485, tak FreeRTOS =RESET  asi overflow;
	// Serial1.begin(9600);

	// swSer.begin(115200);
	// swSer.println("");
}

void loop()
{
	esp_task_wdt_reset();
	ws.cleanupClients();
	// AsyncElegantOTA.loop();
	timer_1ms.update();
	timer_10ms.update();
	timer_100ms.update();
	timer_1sek.update();
	timer_10sek.update();

	TCP_handler(TCP_10001_socket, 10001);
}

void Loop_1ms()
{
	SCT_prud_0 = readADC_Avg();
}

void Loop_10ms()
{
	static uint8_t TimeOut_RXdata = 0;	 // musi byt static lebo sem skaces z Loop
	static uint16_t KolkkoNplnenych = 0; // musi byt static lebo sem skaces z Loop
	static char budd[250];					 // musi byt static lebo sem skaces z Loop

	uint16_t aktualny;
	char temp[200];

	aktualny = 0;
	aktualny = Serial1.available();
	if (aktualny)
	{

		// Serial2.readBytes (temp, aktualny);
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
				// DebugMsgToWebSocket(temp);
				// Serial.printf(temp);

				AIR_PACKET_t *loc_paket;
				loc_paket = (AIR_PACKET_t *)budd;

				sprintf(temp, "[RS485]  DST adresa je:%u\r\n", loc_paket->DSTadress);
				// DebugMsgToWebSocket(temp);
				// Serial.printf(temp);

				sprintf(temp, "[RS485] Mam adresu %u a idem ulozit data z RS485\r\n", loc_paket->SCRadress);
				// DebugMsgToWebSocket(temp);

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

	if (flg.PeriodickyOdosielajZaznamyzBuffera == true && (VratPocetZaznamu(&LogBuffer) != 0))
	{
		OdosliZaznamDosocketu(&LogBuffer);
	}
}

void Loop_100ms(void)
{
}

void Loop_1sek(void)
{
	// ComDebug("[1sek Loop]  mam 1 sek....  ");
	String sprava = rtc.getTime("\r\n[%H:%M:%S] karta a toto cas z PCF8563:");
	sprava += PCFrtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S);
	// unsigned long start = micros();
	// sprava += PCFrtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S);
	// unsigned long end = micros();
	// unsigned long delta = end - start;
	// Serial.print("DELTA PCF8563: ");
	// Serial.println(delta);

	if (digitalRead(SD_CD_pin) == LOW)
	{
		// sprintf(TX_BUF, "[1sek Loop]  karta zasunota\r\n");
		sprava += " Zasunuta";
	}
	else
	{
		// sprintf(TX_BUF, "[1sek Loop]  karta Vysunuta\r\n");
		sprava += " Vysunuta";
	}

	char tt[100];
	sprintf(tt, "   SCTprud: %uA\r\n", SCT_prud_0);
	sprava += tt;
	ComDebugln(sprava);
	// TCP_debugMsg(sprava);
	// sprava.toCharArray(TX_BUF, TX_RX_MAX_BUF_SIZE, 0);
	// send(TCP_10001_socket, (u8 *)TX_BUF, strlen(TX_BUF));

	if (Internet_CasDostupny == false)
	{
		// ComDebug("Internet cas nedostupny !!,  ");
	}
	else
	{
		// ComDebug("Internet cas dostupny,  ");
	}
	// ComDebug("RTC cas cez func rtc.getTime: ");
	// ComDebugln(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
	MyRTC_cas = rtc.getTimeStruct();
	// Serial.print("[1sek Loop]  free Heap je:");
	// Serial.println(ESP.getFreeHeap());

	if (myTimer.socketCloseTimeout)
	{
		if (--myTimer.socketCloseTimeout == 0)
		{
			ComDebugln("[1sek Loop] Zaviram socket len cas uplynul");
			// disconnect(TCP_10001_socket);
			closeSocket(TCP_10001_socket);
		}
	}

	{
		// float testVal = 88.89f;
		// float testVal2 = 99.12f;
		// LogBuffer.zaznam.PosixTime = rtc.getEpoch();
		// LogBuffer.zaznam.zaznamID = IDzaznamu_SCT_prud;
		// float2Bytes(testVal, &LogBuffer.zaznam.data[0]);
		// float2Bytes(testVal2, &LogBuffer.zaznam.data[4]);
		// LogBuffer.zaznam.pocetDat = 8;
		// UlozZaznam(&LogBuffer);
	}
}

void Loop_10sek(void)
{
	static u8_t loc_cnt_10sek = 0;
	String sprava = String("\r\n[10sek Loop]  Mam Loop 10 sek....") + rtc.getDateTime(true);
	Serial.println(sprava);
	// TCP_debugMsg(sprava);
	// DebugMsgToWebSocket("[10sek Loop]  mam 10 sek....\r\n");

	{
		float testVal = 23.456f;
		float testVal2 = 34.567f;
		LogBuffer.zaznam.PosixTime = rtc.getEpoch();
		LogBuffer.zaznam.zaznamID = IDzaznamu_SCT_prud;
		float2Bytes(testVal, &LogBuffer.zaznam.data[0]);
		float2Bytes(testVal2, &LogBuffer.zaznam.data[4]);
		LogBuffer.zaznam.pocetDat = 8;
		UlozZaznam(&LogBuffer);
	}

	// WiFi_connect_sequencer();
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
					 // if (!request->authenticate("ahoj", "xxxx"))
					 // return request->requestAuthentication();
					 // request->send_P(200, "text/html", index_html, processor);
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

					 request->send(200, "text/html", ttt); });

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
	case SOCK_ESTABLISHED:			  /* if connection is established */
		if (getSn_IR(s) & Sn_IR_CON) // toto sa vykona len raz ak zaloziz spojenie
		{
			setSn_IR(s, Sn_IR_CON);
			ComDebugln("spojenie zalozene");

			// TODO tu si nastva myTimer.socketCloseTimeout = 5;
			myTimer.socketCloseTimeout = 0;
			if (LogBuffer.PocetZaznamov)
			{
				ComDebugln(String("Pozor v buffer ma") + LogBuffer.PocetZaznamov +
							  String("zaznamov, musim ich poslat server, ale az pride JSON CAS"));
			}
		}
		if ((size = getSn_RX_RSR(s)) > 0) // Don't need to check SOCKERR_BUSY because it doesn't not occur.
		{
			if (size > TX_RX_MAX_BUF_SIZE)
				size = TX_RX_MAX_BUF_SIZE;
			SDSPI.setFrequency(20000000); // TODO tu mas zmenu frekvencie
			ret = recv(s, (u8 *)ethBuff, size);

			if (ret <= 0)
				return; // check SOCKERR_BUSY & SOCKERR_XXX. For showing the occurrence of SOCKERR_BUSY.
			size = (uint16_t)ret;
			sentsize = 0;

			if (KontrolujBufferZdaObsaujeJSONdata(ethBuff) == true)
			{
				return;
			}

			if (strncmp((const char *)ethBuff, "GET", 3) == 0) // && timers.GET_request_timeout == 0 )
			{
				// SDSPI.setFrequency(35000000);

				sprintf(TX_BUF, "\r\n*****DOSLO GET!!!!");
				unsigned long start = micros();
				// send(s, (u8 *)ethBuff, strlen((const char *)ethBuff));
				unsigned long end = micros();
				unsigned long delta = end - start;
				Serial.print("DELTA: ");
				Serial.println(delta);
				flg.PeriodickyOdosielajZaznamyzBuffera = true;
			}
			if (strncmp((const char *)ethBuff, "STOP", 4) == 0) // && timers.GET_request_timeout == 0 )
			{
				flg.PeriodickyOdosielajZaznamyzBuffera = false;
			}
			if (strncmp((const char *)ethBuff, "POSLI", 5) == 0)
			{
				flg.PeriodickyOdosielajZaznamyzBuffera = true;
				OdosliZaznamDosocketu(&LogBuffer);
				flg.PeriodickyOdosielajZaznamyzBuffera = false;
			}

			// sprintf(TX_BUF, "\r\n*****Test ci ospovida Wiz5100s!");
			// send(s, (u8 *)ethBuff, strlen(ethBuff));
		}
		break;

	case SOCK_CLOSE_WAIT: /* If the client request to close */

		disconnect(s);
		break;

	case SOCK_CLOSED:
		if ((ret = socket(s, Sn_MR_TCP, port, 0x00)) != s)
		{
			ComDebugln("[SOCK_CLOSED] sequencer");
		}
		break;

	case SOCK_INIT: // toto sa vykona od sa close socket a reinicializu, vykona sa to len raz a potom uz len pocuva
		listen(s);
		flg.PeriodickyOdosielajZaznamyzBuffera = false;
		// ComDebugln("[SOCK_INIT] sequencer");
		break;

	default:
		break;
	}
}
//******************************************************************************************************************************
