#include <Arduino.h>
#include "SD.h"
#include <EEPROM.h>
#include "main.h" //kvolu u8,u16..

#include "Pin_assigment.h"
#include "WizChip_my_API.h"
#include "wizchip_conf.h"

//toto spituje retezec napr cas  21:56, ti rozdeli podla delimetru ":"
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
uint32_t AN_Pot1_Buffer[FILTER_LEN] = {0};
int AN_Pot1_i = 0;

uint32_t readADC_Avg(int ADC_Raw)
{
	int i = 0;
	uint32_t Sum = 0;

	AN_Pot1_Buffer[AN_Pot1_i++] = ADC_Raw;
	if (AN_Pot1_i == FILTER_LEN)
	{
		AN_Pot1_i = 0;
	}
	for (i = 0; i < FILTER_LEN; i++)
	{
		Sum += AN_Pot1_Buffer[i];
	}
	return (Sum / FILTER_LEN);
}

static bool Input_digital_filtering(VSTUP_t *input_struct, uint16_t filterCas)
{
	if (digitalRead(input_struct->pin) == LOW)
	{
		//Serial.println("[Input filter] hlasi nula..");
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
		//Serial.println("[Input filter] hlasi jenda..");
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
	//Serial.println("[ScanInputs] begin..");
	bool bolaZmenaVstupu = false;

	DIN[input_SDkarta].zmena = Input_digital_filtering(&DIN[input_SDkarta], filterTime_SD_CD);
	if (DIN[input_SDkarta].zmena == true)
	{
		Serial.print("[ScanInputs] SD CD hlasi  zmenu a to karta: ");
		if (DIN[input_SDkarta].input == 1)
		{
			Serial.println("zasunuta");
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
						Serial.println("=================");
					}
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
		if (DIN[i].zmena == true && DIN[i].input == true)
		{
			DIN[i].counter++;
		} //tu si incrementuju citac impulzu
		bolaZmenaVstupu |= DIN[i].zmena;
	}

	if (bolaZmenaVstupu == true)
	{
		Serial.print("[ScanInputs] hlasi ze mam zmenu na vstupoch....");
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
	pinMode(WIZ_RES_pin, OUTPUT);

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

	rtc.setTime(30, 24, 15, 17, 1, 2021); // 17th Jan 2021 15:24:30

	NaplnWizChipStrukturu();

	SDSPI.setFrequency(35000000);
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
