#include <Arduino.h>
#include <AsyncElegantOTA.h> //https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/#1-basic-elegantota
#include <elegantWebpage.h>

#include <Arduino_JSON.h>
#include <TimeLib.h>
#include "SD.h"
#include <EEPROM.h>
#include "main.h" //kvolu u8,u16..

#include "Pin_assigment.h"
#include "WizChip_my_API.h"
#include "Middleware\Ethernet\wizchip_conf.h"
#include "index.html"
#include "HelpFunction.h"

// toto spituje retezec napr cas  21:56, ti rozdeli podla delimetru ":"
char **split(char **argv, int *argc, char *string, const char delimiter, int allowempty)
{
	*argc = 0;
	do
	{
		if (*string && (*string != delimiter || allowempty))
		{
			argv[(*argc)++] = string;
		}
		while (*string && *string != delimiter)
			string++;
		if (*string)
			*string++ = 0;
		if (!allowempty)
			while (*string && *string == delimiter)
				string++;
	} while (*string);
	return argv;
}

String ConvetWeekDay_UStoCZ(tm *timeInfoPRT)
{
	char loc_buf[30];
	String ee;

	strftime(loc_buf, 30, " %A", timeInfoPRT);
	ee = String(ee + loc_buf);

	if (ee == " Monday")
	{
		ee = "Pondelí";
	}
	else if (ee == " Tuesday")
	{
		ee = "Úterý";
	}
	else if (ee == " Wednesday")
	{
		ee = "Středa";
	}
	else if (ee == " Thursday")
	{
		ee = "Štvrtek";
	}
	else if (ee == " Friday")
	{
		ee = "Pátek";
	}
	else if (ee == " Saturday")
	{
		ee = "Sobota";
	}
	else if (ee == " Sunday")
	{
		ee = "Nedele";
	}
	else
	{
		ee = "? den ?";
	}
	return ee;
}

String ConvetWeekDay_UStoSK(tm *timeInfoPRT)
{
	char loc_buf[30];
	String ee;

	strftime(loc_buf, 30, " %A", timeInfoPRT);
	ee = String(ee + loc_buf);

	if (ee == " Monday")
	{
		ee = "Pondelok";
	}
	else if (ee == " Tuesday")
	{
		ee = "Útorok";
	}
	else if (ee == " Wednesday")
	{
		ee = "Streda";
	}
	else if (ee == " Thursday")
	{
		ee = "Štvrtok";
	}
	else if (ee == " Friday")
	{
		ee = "Piatok";
	}
	else if (ee == " Saturday")
	{
		ee = "Sobota";
	}
	else if (ee == " Sunday")
	{
		ee = "Nedeľa";
	}
	else
	{
		ee = "? den ?";
	}
	return ee;
}

bool SkontrolujCiJePovolenyDenvTyzdni(u8 Obraz, tm *timeInfoPRT)
{
	char loc_buf[30];
	String ee;

	strftime(loc_buf, 30, " %A", timeInfoPRT);
	ee = String(ee + loc_buf);
	u8 vv = 8;
	if (ee == " Monday")
	{
		vv = 0;
	}
	else if (ee == " Tuesday")
	{
		vv = 1;
	}
	else if (ee == " Wednesday")
	{
		vv = 2;
	}
	else if (ee == " Thursday")
	{
		vv = 3;
	}
	else if (ee == " Friday")
	{
		vv = 4;
	}
	else if (ee == " Saturday")
	{
		vv = 5;
	}
	else if (ee == " Sunday")
	{
		vv = 6;
	}

	Serial.print("Ventil ma nastaveny dni:");
	Serial.print(Obraz);
	Serial.print(" a RTC vidi den:");
	Serial.print(vv);

	if (Obraz & (1 << vv))
	{
		return true;
	}

	return false;
}

#include "esp_adc_cal.h"

#define AN_Pot1 35
#define FILTER_LEN 15

u16 readADC_Avg(void)
{
	static u32 AN_acuuVal;
	static u8 AN_SCT_index = 0;
	static u16 Predchozi = 0;

	AN_acuuVal += analogRead(ADC_curren_pin);
	if (++AN_SCT_index == FILTER_LEN)
	{
		Predchozi = (u16)(AN_acuuVal /= FILTER_LEN);
		AN_acuuVal = 0;
		AN_SCT_index = 0;
	}
	return Predchozi;
}

static bool Input_digital_filtering(VSTUP_t *input_struct, uint16_t filterCas)
{
	if (digitalRead(input_struct->pin) == LOW)
	{
		// Serial.println("[Input filter] hlasi nula..");
		if (input_struct->filter < filterCas)
		{
			input_struct->filter++;
		}
		else
		{
			input_struct->input = 1;
		}
	}
	else
	{
		// Serial.println("[Input filter] hlasi jenda..");
		input_struct->input = 0;
		input_struct->filter = 0;
	}
	if (input_struct->input_prew != input_struct->input)
	{
		input_struct->input_prew = input_struct->input;
		return true;
	}
	else
	{
		return false;
	}
}

void ScanInputs(void)
{
	// Serial.println("[ScanInputs] begin..");
	bool bolaZmenaVstupu = false;

	DIN[input_SDkarta].zmena = Input_digital_filtering(&DIN[input_SDkarta], filterTime_SD_CD);
	if (DIN[input_SDkarta].zmena == true)
	{
		Serial.print("[ScanInputs] SD CD hlasi  zmenu a to karta: ");
		if (DIN[input_SDkarta].input == 1)
		{
			Serial.println("zasunuta");
			unsigned long start = micros();
			if (!SD.begin(SD_CS_pin, SDSPI, 10000000))
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

				File profile = SD.open("/aaa.txt", FILE_READ);
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
						Serial.println("================");
					}
					unsigned long end = micros();
					unsigned long delta = end - start;
					Serial.print("DELTA: ");
					Serial.println(delta);
				}
			}
		}
		else
		{
			Serial.println("vysunuta");
			SD.end();
		}
	}
	// Serial.print("[SD CD] input:");
	// Serial.print(DIN[input_SDkarta].input);
	// Serial.print("    inputprew:");
	// Serial.print(DIN[input_SDkarta].input_prew);
	// Serial.print("    inputFilter:");
	// Serial.println(DIN[input_SDkarta].filter);
	DIN[input_SDkarta].input_prew = DIN[input_SDkarta].input;

	for (u8 i = 0; i < pocetDIN; i++)
	{
		DIN[i].zmena = Input_digital_filtering(&DIN[i], filterTime_DI);
		if (DIN[i].zmena == true)
		{
			LogBuffer.zaznam.PosixTime = rtc.getEpoch();
			LogBuffer.zaznam.zaznamID = IDzaznamu_IN1 + i;
			LogBuffer.zaznam.pocetDat = 1;

			if (DIN[i].input == true)
			{
				DIN[i].counter++;
				LogBuffer.zaznam.data[0] = 1;
			} // tu si incrementuju citac impulzu
			else
			{
				LogBuffer.zaznam.data[0] = 0;
			}
			UlozZaznam(&LogBuffer);
			bolaZmenaVstupu |= DIN[i].zmena;
		}
	}

	if (bolaZmenaVstupu == true)
	{
		ComDebugln("[ScanInputs] hlasi ze mam zmenu na vstupoch....");
	}

	// for (u8 u = 0; u < pocetDIN; u++) //toto musi by tu na konci funkcie lebo to nastavi ze aktualny do predchoziho stavu
	// {
	// 	DIN[u].input_prew = DIN[u].input;
	// }
}
void System_init(void)
{
	Serial.print("[Func:System_init]  begin..");
	DIN[input1].pin = DI1_pin;
	DIN[input2].pin = DI2_pin;
	DIN[input3].pin = DI3_pin;
	DIN[input4].pin = DI4_pin;
	DIN[input_SDkarta].pin = SD_CD_pin;

	pinMode(ADC_gain_pin, OUTPUT_OPEN_DRAIN);
	pinMode(SD_CS_pin, OUTPUT);
	pinMode(WIZ_CS_pin, OUTPUT);
	//	pinMode(WIZ_RES_pin, OUTPUT);  //!! POZOR toto ak mas povolene, tak sato furt resetuje !!!

	WizChip_RST_HI();
	WizChip_CS_HI();

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

	RTC_Date Pccc;
	Wire.begin(18, 17);
	PCFrtc.begin();
	// PCFrtc.setDateTime(2019, 4, 1, 12, 33, 59);
	Pccc = PCFrtc.getDateTime();
	rtc.setTime(Pccc.second, Pccc.minute, Pccc.hour, Pccc.day, Pccc.month, Pccc.year); // 17th Jan 2021 15:24:30

	NaplnWizChipStrukturu();

	SDSPI.setFrequency(10000000); // nezabudni ze pri SD.begin(SD_CS_pin, SDSPI,10000000)) budes menit fre na hodnotu v zavorkach
	// SDSPI.setClockDivider(SPI_CLOCK_DIV2);
	SDSPI.begin(SD_sck, SD_miso, SD_mosi, -1);

	// if (!SD.begin(SD_CS_pin, SDSPI))
	// {
	// 	Serial.println("Card Mount Failed");
	// }
	// else
	// {
	// 	Serial.println("Card Mount OK!!...");

	// 	uint8_t cardType = SD.cardType();

	// 	if (cardType == CARD_NONE)
	// 	{
	// 		Serial.println("No SD card attached");
	// 		return;
	// 	}

	// 	Serial.print("SD Card Type: ");
	// 	if (cardType == CARD_MMC)
	// 	{
	// 		Serial.println("MMC");
	// 	}
	// 	else if (cardType == CARD_SD)
	// 	{
	// 		Serial.println("SDSC");
	// 	}
	// 	else if (cardType == CARD_SDHC)
	// 	{
	// 		Serial.println("SDHC");
	// 	}
	// 	else
	// 	{
	// 		Serial.println("UNKNOWN");
	// 	}

	// 	uint64_t cardSize = SD.cardSize() / (1024 * 1024);
	// 	Serial.printf("SD Card Size: %lluMB\n", cardSize);
	// 	Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
	// 	Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
	
	// 	File profile = SD.open("/aaa.txt", FILE_READ);
	// 	Serial.printf("Velkost subora je :%lu\r\n", profile.size());
	// 	if (!profile)
	// 	{
	// 		Serial.println("Opening file to read failed");
	// 	}
	// 	else
	// 	{
	// 		Serial.println("File Content:");

	// 		while (profile.available())
	// 		{
	// 			while (profile.available())
	// 			{
	// 				Serial.write(profile.read());
	// 			}
	// 			Serial.println("");
	// 			Serial.println("File read done");
	// 			Serial.println("=================");
	// 		}
	// 	}
	// }

	WizChip_Reset();
	WizChip_Config(&eth);

	flg.PeriodickyOdosielajZaznamyzBuffera = false;
	Serial.print("[Func:System_init]  end..");
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

	if (NazovSiete[0] != 0xff) // ak mas novy modul tak EEPROM vrati prazdne hodnoty, preto ich neprepisem z EEPROM, ale necham default
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

			// notifyClients();
		}
		else if (strcmp((char *)data, "VratNamerane_TaH") == 0)
		{
			Serial.println("stranky poslali: VratNamerane_TaH ");

			OdosliStrankeVytapeniData();
		}

		else if (strcmp((char *)data, "ZaluzieAllOpen") == 0)
		{
			// Send:02 43 64 00 02 00 0e 80 0e 00 00 00 b8
			u8 loc_buf[14] = {0x2, 0x43, 0x64, 0x0, 0x2, 0x0, 0x0e, 0x80, 0x0e, 0x0, 0x0, 0x0, 0xb8};
			Serial.println("stranky poslali: ZaluzieVsetkyOtvor");

			// Serial1.print("test RS485..Zaluzie All open.. ");
			for (u8 i = 0; i < 13; i++)
			{
				Serial1.write(loc_buf[i]);
			}
			String rr = "[HndlWebSocket] To RS485 posielam OTVOR zaluzie\r\n";
			DebugMsgToWebSocket(rr);
		}
		else if (strcmp((char *)data, "ZaluzieAllStop") == 0)
		{
			// Send:02 43 64 00 02 00 0c 80 0c 00 00 00 bc
			u8 loc_buf[14] = {0x2, 0x43, 0x64, 0x0, 0x2, 0x0, 0x0c, 0x80, 0x0c, 0x0, 0x0, 0x0, 0xbc};
			Serial.println("stranky poslali: ZaluzieAllStop ");

			// Serial1.println("test RS485..Zaluzie All Stop.. ");
			for (u8 i = 0; i < 13; i++)
			{
				Serial1.write(loc_buf[i]);
			}

			String rr = "[HndlWebSocket] To RS485 posielam STOP zaluzie\r\n";
			DebugMsgToWebSocket(rr);
		}

		else if (strcmp((char *)data, "ZaluzieAllClose") == 0)
		{
			// Send:02 43 64 00 02 00 0d 80 0d 00 00 00 ba
			u8 loc_buf[14] = {0x02, 0x43, 0x64, 0x0, 0x2, 0x0, 0x0d, 0x80, 0x0d, 0x0, 0x0, 0x0, 0xba};
			Serial.println("stranky poslali: ZaluzieVsetky zatvor");

			// Serial1.println("test RS485..Zaluzie All close.. ");
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

void WiFi_init(void)
{
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
	Serial.println(WiFi.localIP());

	ws.onEvent(onEvent);		// initWebSocket();
	server.addHandler(&ws); // initWebSocket();

	FuncServer_On();

	AsyncElegantOTA.begin(&server, "qqq", "www"); // Start ElegantOTA

	server.begin();
}

void WiFi_connect_sequencer(void) // vplas kazdych 10 sek loop
{
	static u8_t loc_cnt_10sek = 0;

	Serial.print("Wifi status:");
	Serial.println(WiFi.status());

	// https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
	if (WiFi.status() != WL_CONNECTED)
	{
		loc_cnt_10sek++;
		Internet_CasDostupny = false;
	}
	else
	{
		loc_cnt_10sek = 0;
		Serial.println("[10sek] Parada WIFI je Connect davam loc_cnt na Nula");

		// TODO ak je Wifi connect tak pocitam ze RTC cas bude OK este dorob
		Internet_CasDostupny = true;
		RTC_cas_OK = true;
	}

	if (loc_cnt_10sek == 2)
	{
		Serial.println("[10sek] Odpajam WIFI, lebo wifi nieje: WL_CONNECTED ");
		WiFi.disconnect(1, 1);
	}
	else if (loc_cnt_10sek == 3)
	{
		loc_cnt_10sek = 255;
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
	// server.send (200, "text/html", ttt);
}

String handle_Zadavanie_IP_setting(void)
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
	// Serial.print ("\r\nToto je bufer pre stranky:\r\n");
	// Serial.print(ttt);

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
	// ObjTopeni["tep1"] = room[0].T_vzduch;
	// ObjTopeni["hum1"] = room[0].RH_vlhkkost;

	String jsonString = JSON.stringify(ObjTopeni);
	Serial.print("[ event -VratNamerane_TaH] Odosielam strankam ObjTopeni:");
	// Serial.println(jsonString);
	ws.textAll(jsonString);
}

void TCP_debugMsg(String sprava)
{
#ifdef TCP_debug
	sprava.toCharArray(TX_BUF, TX_RX_MAX_BUF_SIZE, 0);
	send(TCP_10001_socket, (u8 *)TX_BUF, strlen(TX_BUF));
#endif
}

uint8_t VypocitajSumuBuffera(uint8_t *buffer, uint16_t kolko)
{
	uint8_t suma = 0;

	for (uint16_t i = 0; i < kolko; i++)
	{
		suma += buffer[i];
	}

	suma = 255 - suma;
	return suma;
}

uint8_t KontrolaSumyBuffera(uint8_t *buffer, uint16_t kolko)
{
	uint8_t suma = 0;
	for (uint16_t i = 0; i < kolko; i++)
	{
		suma += buffer[i];
	}

	suma = 255 - suma;

	return suma;
}

bool KontrolujBufferZdaObsaujeJSONdata(char JSONbuffer[])
{
	JSONVar myObject = JSON.parse(JSONbuffer);
	{
		if (JSON.typeof(myObject) != "undefined")
		{
			Serial.println("Parichozi JSON SOCKETU mau nejaku deinicu JSONu");
			if (myObject.hasOwnProperty("MACadresa"))
			{
				// ComDebug("myObject ma MAC adresu a to = ");
				// ComDebugln((const char *)myObject["MACadresa"]);
				String dddd;
				dddd = myObject["MACadresa"];
				char *argv[8];
				int argc;
				char str[50];
				dddd.toCharArray(str, sizeof(str), 0);
				split(argv, &argc, str, ':', 0);

				// ComDebugln("-------Toto  su cleny MAC adrese--------\n");
				char pole[50];
				for (int i = 0; i < argc; i++)
				{
					sprintf(pole, "argv[%d] = %s", i, argv[i]);
					// ComDebugln(pole);
					sprintf(pole, "prevedene na int [%d] -", strtol(argv[i], NULL, 16));
					// ComDebugln(pole);
				}
				// ComDebugln("-------------------------\n");

				if (eth.mac[0] == strtol(argv[0], NULL, 16) &&
					 eth.mac[1] == strtol(argv[1], NULL, 16) &&
					 eth.mac[2] == strtol(argv[2], NULL, 16) &&
					 eth.mac[3] == strtol(argv[3], NULL, 16) &&
					 eth.mac[4] == strtol(argv[4], NULL, 16) &&
					 eth.mac[5] == strtol(argv[5], NULL, 16))
				{
					ComDebugln("Super JSON sa rovna mojej MAC adrese");
					myTimer.socketCloseTimeout = 0; // tu je akoze dosiel JSON string kde je MAC
					return true;
				}
			}
			else if (myObject.hasOwnProperty("Cas"))
			{
				//{"Cas":"2021:9:22:13:15:16 "}
				// Ked v JSON dosje "Cas", takto beru ze na druhej strane je opravneny server a mazy flag vynuteneho Close soketu
				myTimer.socketCloseTimeout = 0;
				// flg.PeriodickyOdosielajZaznamyzBuffera = true;

				ComDebug("myObject ma CAS JSON");
				ComDebugln((const char *)myObject["Cas"]);
				String dddd;
				dddd = myObject["Cas"];
				char *argv[10];
				int argc;
				char str[50];
				dddd.toCharArray(str, sizeof(str), 0);
				split(argv, &argc, str, ':', 0);

				ComDebugln("-------Toto  su cleny Casu--------\n");
				char pole[50];
				for (int i = 0; i < argc; i++)
				{
					sprintf(pole, "argv[%d] = %s", i, argv[i]);
					ComDebugln(pole);
					sprintf(pole, "prevedene na int [%d] -", atoi(argv[i]));
					ComDebugln(pole);
				}
				ComDebugln("-------------------------\n");

				int sc, mn, hr, dy, mt, yr;
				sc = atoi(argv[5]);
				mn = atoi(argv[4]);
				hr = atoi(argv[3]);
				dy = atoi(argv[2]);
				mt = atoi(argv[1]);
				yr = atoi(argv[0]);
				if ((sc < 60 && sc > -1) &&
					 (mn > -1 && mn < 60) &&
					 (hr > -1 && hr < 24) &&
					 (dy > 0 && dy < 32) &&
					 (mt > 0 && mt < 13) &&
					 (yr > 2000 && yr < 2500))
				{
					// TODO tu mas uz rozparsrovany RTC, tak si ho uloz kam potrebujes do ESP casu, or do I2C RTC modulu
					RTC_Date Pccc;
					PCFrtc.setDateTime(yr, mt, dy, hr, mn, sc);
					Pccc = PCFrtc.getDateTime();
					rtc.setTime(Pccc.second, Pccc.minute, Pccc.hour, Pccc.day, Pccc.month, Pccc.year); // 17th Jan 2021 15:24:30
				}
				else
				{
					ComDebugln("Error zadal si spatny ca s RTC");
				}
				return true;
			}
		}
		else
		{
			ComDebug("Buffer nema JSON paket");
		}
	}
	return false;
}

uint16_t Read_u16_Value(char *buff)
{
	uint16_t *ptr = (uint16_t *)buff;
	return *ptr;
	/*char *ptr;
	ptr = buff+1;
	uint16_t loc_hodnota;
	loc_hodnota  = *ptr; ptr--;
	loc_hodnota <<= 8;
	loc_hodnota += *ptr;
	return loc_hodnota;*/
}

uint32_t Read_u32_Value(char *buff)
{
	uint32_t *ptr = (uint32_t *)buff;
	return *ptr;
	/*char *ptr;
	ptr = buff;
	ptr += 3;
	uint32_t loc_hodnota;
	loc_hodnota  = *ptr;  loc_hodnota <<= 8; ptr--;
	loc_hodnota  += *ptr; loc_hodnota <<= 8; ptr--;
	loc_hodnota  += *ptr; loc_hodnota <<= 8; ptr--;
	loc_hodnota += *ptr;
	return loc_hodnota;*/
}

int32_t Read_32_Value(char *buff)
{
	int32_t *ptr = (int32_t *)buff;
	return *ptr;
}

// TODO tu si otestuj este nacitani 64bit val lebo na STM32 to vytuhovalo inakpouzi cez memcpy dole odkomentovanu
//  void Read_u64_Value(char *buff, char *data)
//  {
//     memcpy(data, buff, sizeof(uint64_t));
//  }
u64 Read_u64_Value(char *buff)
{
	uint64_t *ptr = (uint64_t *)buff;
	return *ptr;
}

void Swap_float(float *v)
{
	char *c = (char *)v;
	char a = c[0];
	c[0] = c[3];
	c[3] = a;
	a = c[1];
	c[1] = c[2];
	c[2] = a;
}

float Read_Float_Value(char *buff)
{
	float *ptr = (float *)buff;
	return *ptr;
}

void float2Bytes(float val, uint8_t *bytes_array)
{
	// Create union of shared memory space
	union
	{
		float float_variable;
		uint8_t temp_array[4];
	} u;
	// Overite bytes of union with float variable
	u.float_variable = val;
	// Assign bytes to input array
	memcpy(bytes_array, u.temp_array, 4);
}

void Double2Bytes(double val, uint8_t *bytes_array)
{
	uint8_t loc_buff[8];
	// Create union of shared memory space
	union
	{
		double float_variable;
		uint8_t temp_array[8];
	} u;
	// Overite bytes of union with float variable
	u.float_variable = val;
	// Assign bytes to input array
	memcpy(bytes_array, u.temp_array, 8);
}

/**/
bool OdosliZaznamDosocketu(LOGBUFF_t *logBuffStruc)
{
	char locWorkBuff[250];
	if (flg.PeriodickyOdosielajZaznamyzBuffera == true)
	{
		String sprava = VyberZaznam(logBuffStruc, false);
		// ComDebugln("Toto idem poslat do Socketu");
		// ComDebugln(sprava);
		sprava.toCharArray(locWorkBuff, sizeof(locWorkBuff), 0);
		i32 ret = send(TCP_10001_socket, (u8 *)locWorkBuff, strlen(locWorkBuff));
		// ComDebug(String("Do  TCP soketu sa malo poslat  bytes: ") + strlen(locWorkBuff) + String(" a poslalo sa:") + ret);
		if (strlen(locWorkBuff) == ret)
		{
			// ComDebug("Odoslanie dat do TCP socketu bolo OK, tak zaznam z buffer mazem");
			// ComDebugln(ret);
			VyberZaznam(logBuffStruc, true); // zmazem zaznam
			return true;
		}
		else
		{
			// ComDebug("Odoslanie dat do TCP socketu bolo NOK!!  zaznam necham v bufferi");
		}
	}
	return false;
}

bool UlozZaznam(LOGBUFF_t *logBuffStruc)
{
	if ((((logBuffStruc->zaznam.pocetDat + 5) + logBuffStruc->BufferIndex) >= maxVelkostLogBuffera) ||
		 (logBuffStruc->PocetZaznamov >= maxPocetZaznamov))
	{
		ComDebugln(String("Pozor plny buffer, posuvam zaznami pred je pocet") + logBuffStruc->PocetZaznamov + String("index v bufferi:") + logBuffStruc->BufferIndex);
		VyberZaznam(logBuffStruc, true);
		ComDebugln(String("Po posunutije pocet") + logBuffStruc->PocetZaznamov + String("index v bufferi:") + logBuffStruc->BufferIndex);
	}

	logBuffStruc->AdresList[logBuffStruc->PocetZaznamov] = logBuffStruc->BufferIndex;

	uint8_t *ptrBuffer;
	long epoch = logBuffStruc->zaznam.PosixTime;
	logBuffStruc->Buffer[logBuffStruc->BufferIndex++] = epoch & 0xff;
	logBuffStruc->Buffer[logBuffStruc->BufferIndex++] = epoch >> 8;
	logBuffStruc->Buffer[logBuffStruc->BufferIndex++] = epoch >> 16;
	logBuffStruc->Buffer[logBuffStruc->BufferIndex++] = epoch >> 24;
	logBuffStruc->Buffer[logBuffStruc->BufferIndex++] = logBuffStruc->zaznam.zaznamID;

	ptrBuffer = (uint8_t *)logBuffStruc->zaznam.data;
	for (uint8_t i = 0; i < logBuffStruc->zaznam.pocetDat; i++)
	{
		logBuffStruc->Buffer[logBuffStruc->BufferIndex++] = ptrBuffer[i];
	}

	logBuffStruc->PocetZaznamov++;
	ComDebugln(String("Ukladam zaznam:") + logBuffStruc->PocetZaznamov + String("index v bufferi:") + logBuffStruc->BufferIndex);
	return true;
}

String VyberZaznam(LOGBUFF_t *logBuffStruc, bool aZrovnaZaznamZmaz)
{
	static u8 millisekundy = 0;
	uint16_t PocetBytesZaznamu = logBuffStruc->AdresList[1] - logBuffStruc->AdresList[0];
	if (logBuffStruc->PocetZaznamov == 1)
	{
		PocetBytesZaznamu = logBuffStruc->BufferIndex;
	}
	char locBuff[110];
	memset(locBuff, 0, sizeof(locBuff));
	for (uint16_t i = 0; i < PocetBytesZaznamu; i++)
	{
		locBuff[i] = logBuffStruc->Buffer[i];
	}
	// printf("Spracuvam zaznam co ma pocet bytes:%u  a to :%s\r\n", PocetBytesZaznamu, locBuff);

	if (aZrovnaZaznamZmaz == true)
	{
		memcpy(&logBuffStruc->Buffer[0], &logBuffStruc->Buffer[PocetBytesZaznamu], (logBuffStruc->BufferIndex - PocetBytesZaznamu));
		memcpy(&logBuffStruc->AdresList[0], &logBuffStruc->AdresList[1], (logBuffStruc->PocetZaznamov * 2));
		logBuffStruc->AdresList[0] = 0;
		logBuffStruc->PocetZaznamov--;
		for (uint16_t i = 1; i < logBuffStruc->PocetZaznamov; i++)
		{
			logBuffStruc->AdresList[i] -= PocetBytesZaznamu;
		}
		logBuffStruc->BufferIndex -= PocetBytesZaznamu;

		// ComDebugln(String("Vyber zaznam -s MAZANIM!! zosrava zaznamu:") + logBuffStruc->PocetZaznamov);
	}
	else
	{
		// ComDebugln("Vyber zaznam BEZ zmazania");
	}
	long temp = locBuff[3];
	temp <<= 8; // stol(epoch);
	temp += locBuff[2];
	temp <<= 8;
	temp += locBuff[1];
	temp <<= 8;
	temp += locBuff[0];

	String StrCas;
	StrCas = String("") + year(temp) +
				String(":") + month(temp) +
				String(":") + day(temp) +
				String(":") + hour(temp) +
				String(":") + minute(temp) +
				String(":") + second(temp) +
				String(":") + millisekundy;
	if (++millisekundy == 100)
	{
		millisekundy = 0;
	}

	String IDzazna = "NOK";
	String StrVall = "---";
	float Val1, Val2;
	switch (locBuff[4])
	{
	case IDzaznamu_SCT_prud:
	{
		IDzazna = "AI1";
		Val1 = Read_Float_Value(&locBuff[5]);
		Val2 = Read_Float_Value(&locBuff[9]);
		StrVall = String(Val1, 1) + String(":") + String(Val2, 1);
	}
	break;
	case IDzaznamu_IN1:
	{
		IDzazna = "IN1";
		u8 Vall = locBuff[5];
		StrVall = String(Vall);
	}
	break;
	case IDzaznamu_IN2:
	{
		IDzazna = "IN2";
		u8 Vall = locBuff[5];
		StrVall = String(Vall);
	}
	break;
	case IDzaznamu_IN3:
	{
		IDzazna = "IN3";
		u8 Vall = locBuff[5];
		StrVall = String(Vall);
	}
	break;
	case IDzaznamu_IN4:
	{
		IDzazna = "IN4";
		u8 Vall = locBuff[5];
		StrVall = String(Vall);
	}
	break;
	case IDzaznamu_SCT_test:
	{
		IDzazna = "Test1";
		Val1 = Read_Float_Value(&locBuff[5]);
		StrVall = String(Val1, 1);
	}
	break;

	default:
		break;
	}

	JSONVar tempObject;
	tempObject["Cas"] = StrCas;
	tempObject[IDzazna] = StrVall;

	String sprava;
	sprava = JSON.stringify(tempObject);
	sprava += String("\r\n");
	return sprava;
}

u16 VratPocetZaznamu(LOGBUFF_t *logBuffStruc)
{
	return logBuffStruc->PocetZaznamov;
}
