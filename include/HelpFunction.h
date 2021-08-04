#ifndef __HelpFunction_H
#define __HelpFunction_H

#include <Arduino.h>

char **split(char **argv, int *argc, char *string, const char delimiter, int allowempty);

String ConvetWeekDay_UStoCZ(tm *timeInfoPRT);
String ConvetWeekDay_UStoSK(tm *timeInfoPRT);
bool SkontrolujCiJePovolenyDenvTyzdni ( u8 Obraz, tm *timeInfoPRT );
uint32_t readADC_Avg(int ADC_Raw);
#endif