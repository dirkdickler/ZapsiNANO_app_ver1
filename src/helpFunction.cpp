#include <Arduino.h>
#include "define.h"
#include "main.h" //kvolu u8,u16..

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

bool Input_digital_filtering(VSTUP_t *input_struct, uint16_t filterCas)
{
	if (digitalRead(input_struct->pin) == LOW)
	{
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
		input_struct->input = 0;
		input_struct->filter = 0;
	}
	if (input_struct->input_prew != input_struct->input)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ScanInputs(void)
{
	bool bolaZmenaVstupu = false;

	for (u8 i = 0; i < pocetDIN; i++)
	{
		DIN[i].zmena = Input_digital_filtering(&DIN[i], filterTime_DI);
		if (DIN[i].zmena && DIN[i].input == 1)
		{
			DIN[i].counter++;
		} //tu si incrementuju citac impulzu
		bolaZmenaVstupu |= DIN[i].zmena;
	}

	for (u8 u = 0; u < pocetDIN; u++)
	{
		DIN[u].input_prew = DIN[u].input;
	}
}