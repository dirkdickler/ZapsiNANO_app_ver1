#include "Arduino.h"
#include "WizChip_my_API.h"
#include "main.h"

void WizChip_sendSPI_byte(uint8_t znak)
{
	SDSPI.transfer(znak);
}

uint8_t WizChip_SPI_read_byte(void)
{
	return SDSPI.transfer(0);
}
void WizChip_WriteSPI_burst (uint8_t *buff, uint16_t len)
{
	SDSPI.transferBytes(buff,NULL,len);	
}

void  WizChip_ReakSPI_bust (uint8_t *buff, uint16_t len)
{
	SDSPI.transferBytes(NULL,buff ,len);	
}

void wizchip_select(void)
{
	WizChip_CS_LO();
}

void wizchip_deselect(void)
{
	WizChip_CS_HI();
}

void wizchip_write(uint8_t wb)
{
	WizChip_sendSPI_byte(wb);
}

uint8_t wizchip_read(void)
{
	return WizChip_SPI_read_byte();
}

void WizChip_Reset(void)
{
	WizChip_RST_LO();
	delay(50); 
	WizChip_RST_HI();
	delay(50); 
	

	WizChip_sendSPI_byte(0xff);
	delay(500); 
	wizchip_deselect();
}

void WizChip_Config(wiz_NetInfo *ethh)
{
	ctlnetwork(CN_SET_NETINFO, (void *)ethh);
	ctlnetwork(CN_GET_NETINFO, (void *)ethh);
}
