#include <AsyncElegantOTA.h> //https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/#1-basic-elegantota
#include <elegantWebpage.h>
#include <Hash.h>
//#include <webserial_webpage.h>
/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-websocket-server-arduino/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

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
u8 RS485_toRx_timeout;
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
bool RTC_cas_OK = false;		   //ze mam RTC fakt nastaveny bud z interneru, alebo nastaveny manualne
								   //a to teda ze v RTC mam fakr realny cas
								   //Tento FLAG, nastavi len pri nacitanie casu z internutu, alebo do buducna manualne nastavenie casu cew WEB

const u8 mojePrmenaae = 25;
u16_t cnt = 0;

static TERMOSTAT_t room[12];
char gloBuff[200];
bool LogEnebleWebPage = false;

VSTUP_t DIN[pocetDIN];

void notifyClients()
{
	//ws.textAll(String(ledState));
}

//** ked Webstrenaky - jejich ws posle nejake data napriklad "VratMiCas" tj ze strnaky chcu RTC aby ich napriklad zobrazili
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
	AwsFrameInfo *info = (AwsFrameInfo *)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		data[len] = 0;

		if (strcmp((char *)data, "VratMiCas") == 0)
		{
			OdosliCasDoWS();

			//notifyClients();
		}
		else if (strcmp((char *)data, "VratNamerane_TaH") == 0)
		{
			Serial.println("stranky poslali: VratNamerane_TaH ");

			OdosliStrankeVytapeniData();
		}

		else if (strcmp((char *)data, "ZaluzieAllOpen") == 0)
		{
			//Send:02 43 64 00 02 00 0e 80 0e 00 00 00 b8
			u8 loc_buf[14] = {0x2, 0x43, 0x64, 0x0, 0x2, 0x0, 0x0e, 0x80, 0x0e, 0x0, 0x0, 0x0, 0xb8};
			Serial.println("stranky poslali: ZaluzieVsetkyOtvor");

			//Serial1.print("test RS485..Zaluzie All open.. ");
			for (u8 i = 0; i < 13; i++)
			{
				Serial1.write(loc_buf[i]);
			}
			String rr = "[HndlWebSocket] To RS485 posielam OTVOR zaluzie\r\n";
			DebugMsgToWebSocket(rr);
		}
		else if (strcmp((char *)data, "ZaluzieAllStop") == 0)
		{
			//Send:02 43 64 00 02 00 0c 80 0c 00 00 00 bc
			u8 loc_buf[14] = {0x2, 0x43, 0x64, 0x0, 0x2, 0x0, 0x0c, 0x80, 0x0c, 0x0, 0x0, 0x0, 0xbc};
			Serial.println("stranky poslali: ZaluzieAllStop ");

			//Serial1.println("test RS485..Zaluzie All Stop.. ");
			for (u8 i = 0; i < 13; i++)
			{
				Serial1.write(loc_buf[i]);
			}

			String rr = "[HndlWebSocket] To RS485 posielam STOP zaluzie\r\n";
			DebugMsgToWebSocket(rr);
		}

		else if (strcmp((char *)data, "ZaluzieAllClose") == 0)
		{
			//Send:02 43 64 00 02 00 0d 80 0d 00 00 00 ba
			u8 loc_buf[14] = {0x02, 0x43, 0x64, 0x0, 0x2, 0x0, 0x0d, 0x80, 0x0d, 0x0, 0x0, 0x0, 0xba};
			Serial.println("stranky poslali: ZaluzieVsetky zatvor");

			//Serial1.println("test RS485..Zaluzie All close.. ");
			for (u8 i = 0; i < 13; i++)
			{
				Serial1.write(loc_buf[i]);
			}
			String rr = "[HndlWebSocket] To RS485 posielam ZATVOR zaluzie\r\n";
			DebugMsgToWebSocket(rr);
		}
	}
}

void onEvent(AsyncWebSocket *server,
			 AsyncWebSocketClient *client,
			 AwsEventType type,
			 void *arg,
			 uint8_t *data,
			 size_t len)
{
	switch (type)
	{
	case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		break;
	case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		break;
	case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
		break;
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	}
}

void initWebSocket()
{
	ws.onEvent(onEvent);
	server.addHandler(&ws);
}

String processor(const String &var)
{
	Serial.println(var);
	if (var == "STATE")
	{
		/*if (ledState) {
			return "ON";
		}
		else {
			return "OFF";
		}*/
	}

	return "--";
}

/**********************************************************
 ***************        SETUP         **************
 **********************************************************/

void setup()
{
	Serial.begin(115200);
	Serial.println("Spustam applikaciu.1222");
	System_init();
	//attachInterrupt(digitalPinToInterrupt(ENCODER1), encoder, RISING);
	//pinMode(ENCODER1, INPUT);
	//pinMode(ENCODER2, INPUT);

	rtc.setTime(30, 24, 15, 17, 1, 2021); // 17th Jan 2021 15:24:30

	SDSPI.setFrequency(3500000);
	SDSPI.begin(SD_sck, SD_miso, SD_mosi, -1);

	if (!SD.begin(SD_CS_pin, SDSPI))
	{
		Serial.println("Card Mount Failed");
	}
	else
	{
		Serial.println("Card Mount OK!!...");

		uint8_t cardType = SD.cardType();

		if (cardType == CARD_NONE)
		{
			Serial.println("No SD card attached");
			return;
		}

		Serial.print("SD Card Type: ");
		if (cardType == CARD_MMC)
		{
			Serial.println("MMC");
		}
		else if (cardType == CARD_SD)
		{
			Serial.println("SDSC");
		}
		else if (cardType == CARD_SDHC)
		{
			Serial.println("SDHC");
		}
		else
		{
			Serial.println("UNKNOWN");
		}

		uint64_t cardSize = SD.cardSize() / (1024 * 1024);
		Serial.printf("SD Card Size: %lluMB\n", cardSize);
		Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
		Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));

		File profile = SD.open("/IPCONFIG.TXT", FILE_READ);
		Serial.printf("Velkost subora je :%lu\r\n", profile.size());
		if (!profile)
		{
			Serial.println("Opening file to read failed");
		}
		else
		{
			Serial.println("File Content:");

			while (profile.available())
			{
				while (profile.available())
				{
					Serial.write(profile.read());
				}
				Serial.println("");
				Serial.println("File read done");
				Serial.println("=================");
			}
		}
	}

	ESPinfo();

	myObject["hello"] = " 11:22 Streda";
	myObject["true"] = true;
	myObject["x"] = 42;
	myObject2["Citac"] = 42;

	NacitajEEPROM_setting();
	//WiFi.mode(WIFI_MODE_STA);
	WiFi.mode(WIFI_MODE_APSTA);
	Serial.println("Creating Accesspoint");
	WiFi.softAP(soft_ap_ssid, soft_ap_password, 7, 0, 3);
	Serial.print("IP address:\t");
	Serial.println(WiFi.softAPIP());

	if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
	{
		Serial.println("STA Failed to configure");
	}

	WiFi.begin(NazovSiete, Heslo);
	u8_t aa = 0;
	while (WiFi.waitForConnectResult() != WL_CONNECTED && aa < 2)
	{
		Serial.print(".");
		aa++;
	}
	// Print ESP Local IP Address
	Serial.println(WiFi.localIP()); //TODO test todo

	initWebSocket();

	FuncServer_On();

	AsyncElegantOTA.begin(&server, "qqq", "www"); // Start ElegantOTA

	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
	//getLocalTime(&MyRTC_cas);
	server.begin();

	timer_1ms.start();
	timer_10ms.start();
	timer_100ms.start();
	timer_1sek.start();
	timer_10sek.start();
	esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
	esp_task_wdt_add(NULL);				  //add current thread to WDT watch

	testik();

	//RS485 musis spustit az tu, lebo ak ju das hore a ESP ceka na konnect wifi, a pridu nejake data na RS485, tak FreeRTOS =RESET  asi overflow;
	Serial1.begin(9600);

	swSer.begin(115200);
	swSer.println("");
}

void loop()
{
	esp_task_wdt_reset();
	ws.cleanupClients();
	AsyncElegantOTA.loop();
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
				DebugMsgToWebSocket(temp);
				//Serial.printf(temp);

				RS485_PACKET_t *loc_paket;
				loc_paket = (RS485_PACKET_t *)budd;

				sprintf(temp, "[RS485]  DST adresa je:%u\r\n", loc_paket->DSTadress);
				DebugMsgToWebSocket(temp);
				//Serial.printf(temp);

				sprintf(temp, "[RS485] Mam adresu %u a idem ulozit data z RS485\r\n", loc_paket->SCRadress);
				DebugMsgToWebSocket(temp);

				if (loc_paket->SCRadress == 10)
				{
					OdosliStrankeVytapeniData();
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

	String rr = "[1sek Loop] signalu: " + (String)WiFi.RSSI() + "dBm  a Heap: " + (String)ESP.getFreeHeap() + " kB\r\n";
	DebugMsgToWebSocket(rr);
}

void Loop_10sek(void)
{
	static u8_t loc_cnt = 0;
	//Serial.println("\r\n[10sek Loop]  Mam Loop 10 sek..........");
	DebugMsgToWebSocket("[10sek Loop]  mam 1 sek....\r\n");
	Serial.print("Wifi status:");
	Serial.println(WiFi.status());

	//https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
	if (WiFi.status() != WL_CONNECTED)
	{
		loc_cnt++;
		Internet_CasDostupny = false;
	}
	else
	{
		loc_cnt = 0;
		Serial.println("[10sek] Parada WIFI je Connect davam loc_cnt na Nula");

		//TODO ak je Wifi connect tak pocitam ze RTC cas bude OK este dorob
		Internet_CasDostupny = true;
		RTC_cas_OK = true;
	}

	if (loc_cnt == 2)
	{
		Serial.println("[10sek] Odpajam WIFI, lebo wifi nieje: WL_CONNECTED ");
		WiFi.disconnect(1, 1);
	}

	if (loc_cnt == 3)
	{
		loc_cnt = 255;
		WiFi.mode(WIFI_MODE_APSTA);
		Serial.println("znovu -Creating Accesspoint");
		WiFi.softAP(soft_ap_ssid, soft_ap_password, 7, 0, 3);

		if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
		{
			Serial.println("STA Failed to configure");
		}
		Serial.println("znovu -Wifi begin");
		WiFi.begin(NazovSiete, Heslo);
		u8_t aa = 0;
		while (WiFi.waitForConnectResult() != WL_CONNECTED && aa < 2)
		{
			Serial.print(".");
			aa++;
		}
	}
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
// #include <Arduino.h>
// #include "define.h"

void testik(void)
{
	Serial.println("Test funkcie testik");

	Serial.println("Konec - Test funkcie testik");
}

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

int8_t NacitajEEPROM_setting(void)
{
	if (!EEPROM.begin(500))
	{
		Serial.println("Failed to initialise EEPROM");
		return -1;
	}

	Serial.println("Succes to initialise EEPROM");

	EEPROM.readBytes(EE_NazovSiete, NazovSiete, 16);

	if (NazovSiete[0] != 0xff) //ak mas novy modul tak EEPROM vrati prazdne hodnoty, preto ich neprepisem z EEPROM, ale necham default
	{
		String apipch = EEPROM.readString(EE_IPadresa); // "192.168.1.11";
		local_IP = str2IP(apipch);

		apipch = EEPROM.readString(EE_SUBNET);
		subnet = str2IP(apipch);

		apipch = EEPROM.readString(EE_Brana);
		gateway = str2IP(apipch);

		memset(NazovSiete, 0, sizeof(NazovSiete));
		memset(Heslo, 0, sizeof(Heslo));
		u8_t dd = EEPROM.readBytes(EE_NazovSiete, NazovSiete, 16);
		u8_t ww = EEPROM.readBytes(EE_Heslosiete, Heslo, 20);
		Serial.printf("Nacitany nazov siete a heslo z EEPROM: %s  a %s\r\n", NazovSiete, Heslo);
		return 0;
	}
	else
	{
		Serial.println("EEPROM je este prazna, nachavma default hodnoty");
		sprintf(NazovSiete, "semiart");
		sprintf(Heslo, "aabbccddff");
		return 1;
	}
}

void OdosliStrankeVytapeniData(void)
{
	ObjTopeni["tep1"] = room[0].T_vzduch;
	ObjTopeni["hum1"] = room[0].RH_vlhkkost;
	ObjTopeni["tep2"] = room[1].T_vzduch;
	ObjTopeni["hum2"] = room[1].RH_vlhkkost;
	ObjTopeni["tep3"] = room[2].T_vzduch;
	ObjTopeni["hum3"] = room[2].RH_vlhkkost;
	ObjTopeni["tep4"] = room[3].T_vzduch;
	ObjTopeni["hum4"] = room[3].RH_vlhkkost;
	ObjTopeni["tep5"] = room[4].T_vzduch;
	ObjTopeni["hum5"] = room[4].RH_vlhkkost;
	ObjTopeni["tep6"] = room[5].T_vzduch;
	ObjTopeni["hum6"] = room[5].RH_vlhkkost;
	ObjTopeni["tep7"] = room[6].T_vzduch;
	ObjTopeni["hum7"] = room[6].RH_vlhkkost;
	ObjTopeni["tep8"] = room[7].T_vzduch;
	ObjTopeni["hum8"] = room[7].RH_vlhkkost;
	ObjTopeni["tep9"] = room[8].T_vzduch;
	ObjTopeni["hum9"] = room[8].RH_vlhkkost;
	ObjTopeni["tep10"] = room[9].T_vzduch;
	ObjTopeni["hum10"] = room[9].RH_vlhkkost;
	ObjTopeni["tep11"] = room[10].T_vzduch;
	ObjTopeni["hum11"] = room[10].RH_vlhkkost;

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

void System_init(void)
{
	Serial.print("[Func:System_init]  begin..");
    DIN[input1].pin = DI1_pin; 
	DIN[input2].pin = DI2_pin; 
	DIN[input3].pin = DI3_pin; 
	DIN[input4].pin = DI4_pin; 
	


	pinMode(ADC_gain_pin, OUTPUT_OPEN_DRAIN);
	pinMode(SD_CS_pin, OUTPUT);
	pinMode(WIZ_CS_pin, OUTPUT);
	pinMode(WIZ_RES_pin, OUTPUT);

	pinMode(DI1_pin, INPUT_PULLUP);
	pinMode(DI2_pin, INPUT_PULLUP);
	pinMode(DI3_pin, INPUT_PULLUP);
	pinMode(DI4_pin, INPUT_PULLUP);
	pinMode(SD_CD_pin, INPUT_PULLUP);
	pinMode(WIZ_INT_pin, INPUT_PULLUP);
	pinMode(Joy_up_pin, INPUT_PULLUP);
	pinMode(Joy_dn_pin, INPUT_PULLUP);
	pinMode(Joy_Butt_pin, INPUT_PULLUP);
	pinMode(Joy_left_pin, INPUT_PULLUP);
	pinMode(Joy_right_pin, INPUT_PULLUP);
	//digitalWrite(LedOrange, LOW);

	Serial.print("[Func:System_init]  end..");
}