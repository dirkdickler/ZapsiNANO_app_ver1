
#include "WizChip_my_API.h"

void WizChip_sendSPI_byte(uint8_t znak)
{
	uint8_t ptrData[10];
	ptrData[0] = znak;
	//HAL_SPI_Transmit(&hspi2, ptrData, 1, 1000);
}

uint8_t WizChip_SPI_read_byte(void)
{
	uint8_t ptrData[10];
	//ptrData[0] = znak;
	//HAL_SPI_Receive(&hspi2, ptrData, 1, 1000);
	return ptrData[0];
}

void  wizchip_select(void)
{
	WizChip_CS_LO();
}

void  wizchip_deselect(void)
{
	WizChip_CS_HI();
}

void  wizchip_write(uint8_t wb)
{
    WizChip_sendSPI_byte(wb);
}

uint8_t wizchip_read( void )
{
	return WizChip_SPI_read_byte();
}

void WizChip_Reset(void)
{
	WizChip_RST_LO();
	//HAL_Delay(50);
	WizChip_RST_HI();
	//HAL_Delay(50);
	WizChip_sendSPI_byte(0xff);
	//HAL_Delay(50);
	wizchip_deselect();

}

void WizChip_Config( wiz_NetInfo *ethh)
{
	ctlnetwork(CN_SET_NETINFO, (void*)ethh);
	ctlnetwork(CN_GET_NETINFO, (void*)ethh);
}



