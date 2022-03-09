/*
 * max22190.c
 *
 * Created: 19.01.2021 07:18:29
 *  Author: gfcwfzkm
 */ 

#include "max22190.h"

#define CRC_MASK	0x1F

/* Calculate 5-bit CRC, code originally by Maxim, slightly modified.
 * Original Source:
 * https://www.maximintegrated.com/en/design/technical-documents/app-notes/6/6798.html
 * Last 5 bits of 24-bit frame are discarded, 19-bit data length
 * Polynomial P(x) = x5+x4+x2+x0 -> 110101
 * Init word append to the 19-bit data -> 00111
 * "data[0]" is the MSB Byte
 * "data[1]" is the Middle Byte
 * "data[2]" is the LSB Byte
 * returns a 5-bit CRC checksum
 */
uint8_t max22_getCRC(uint8_t *data)
{
	const uint8_t length = 19;		//19-bit data
	const uint8_t crc_init = 0x07;	//5-bit init word, constant, 00111
	const uint8_t crc_poly = 0x35;	//6-bit polynomial, constant, 110101
	uint8_t loopCnt;
	uint8_t crc_step;
	uint8_t crc_result;
	uint8_t tmp;
	
	//construct 24-bit data frame
	uint32_t datainput = (uint32_t)data[0] << 16 | (uint32_t)data[1] << 8 | data[2];
	//append 5-bit init word to first 19-bit data
	datainput = (datainput & 0xffffe0) + crc_init;
	
	//first step, get crc_step 0
	tmp = (uint8_t)((datainput & 0xfc0000) >> 18); //crc_step 0= data[18:13]
	//next crc_step = crc_step[5] = 0 ? (crc_step[5:0] ^ crc_poly) : crc_step[5:0]
	if ((tmp & 0x20) == 0x20)
	{
		crc_step = (uint8_t)(tmp ^ crc_poly);
	}
	else
	{
		crc_step = tmp;
	}
	//step 1-18
	for (loopCnt = 0; loopCnt < length - 1; loopCnt++)
	{
		//append next data bit to previous crc_step[4:0], {crc_step[4:0], next data bit}
		tmp = (uint8_t)(((crc_step & 0x1f) << 1) + ((datainput >> (length - 2 - loopCnt)) & 0x01));
		//next crc_step = crc_step[5] = 0 ? (crc_step[5:0] ^ crc_poly) : crc_step[5:0]
		if ((tmp & 0x20) == 0x20)
		{
			crc_step = (uint8_t)(tmp ^ crc_poly);
		}
		else
		{
			crc_step = tmp;
		}
	}
	crc_result = (uint8_t)(crc_step & 0x1f); //crc result = crc_step[4:0]
	return crc_result;
}

void max22_initStruct(max22_t *din, void *ioComs,
				uint8_t (*startTransaction)(void*),
				uint8_t (*transceiveBytes)(void*,uint8_t,uint8_t*,uint16_t),
				uint8_t (*endTransaction)(void*))
{
	din->ioInterface = ioComs;
	din->startTransaction = startTransaction;
	din->transceiveBytes = transceiveBytes;
	din->endTransaction = endTransaction;
}

enum MAX22_error max22_init(max22_t *din, enum MAX22_CRCEN crcen)
{
	uint8_t tempVal;
	
	din->crc_en = crcen;
	din->coms_error = MAX22_NOERROR;
	
	// Set the device to default register settings, unless POR is set
	tempVal = max22_readReg(din, MAX22_R_FAULT1);
	if (!(tempVal & MAX22_R_FAULT1_POR))
	{
		max22_writeReg(din, MAX22_R_WIREBREAK, 0);
		max22_setInputFilter(din, 0xFF, MAX22_FILTER_BYPASS);
		max22_writeReg(din, MAX22_R_INEN, 0xFF);
		max22_writeReg(din, MAX22_R_FAULT1EN, MAX22_R_FAULT1_CRC | MAX22_R_FAULT1_POR);
		max22_writeReg(din, MAX22_R_FAULT1, 0);
	}
	
	// Checking if we really are connected to the device, by reading a default-value back
	tempVal = max22_readReg(din, MAX22_R_FAULT1EN);
	if (tempVal != (MAX22_R_FAULT1_CRC | MAX22_R_FAULT1_POR))	din->coms_error |= MAX22_ERROR_COMS;
	
	return din->coms_error;
}

uint8_t max22_readReg(max22_t *din, uint8_t regAddr)
{
	uint8_t dBuf[3];
	uint8_t dLen = 2;
	
	dBuf[0] = regAddr | MAX22_C_READ;
	dBuf[1] = 0;
	dBuf[2] = 0;
	
	if (din->crc_en == MAX22_CRCEN)
	{
		dBuf[2] = max22_getCRC(dBuf);
		dLen = 3;
	}
	
	if (din->startTransaction(din->ioInterface))				din->coms_error |= MAX22_ERROR_COMS;
	if (din->transceiveBytes(din->ioInterface, 0, dBuf, dLen))	din->coms_error |= MAX22_ERROR_COMS;
	if (din->endTransaction(din->ioInterface))					din->coms_error |= MAX22_ERROR_COMS;
	
	if ( (din->crc_en == MAX22_CRCEN) && (max22_getCRC(dBuf) != (dBuf[2] & CRC_MASK)) )
	{
		din->coms_error |= MAX22_ERROR_CRC;
	}
		
	din->InputData = dBuf[0];
		
	return dBuf[1];
}

void max22_writeReg(max22_t *din, uint8_t regAddr, uint8_t val)
{
	uint8_t dBuf[3];
	uint8_t dLen = 2;
	
	dBuf[0] = regAddr | MAX22_C_WRITE;
	dBuf[1] = val;
	dBuf[2] = 0;
	
	if (din->crc_en == MAX22_CRCEN)
	{
		dBuf[2] = max22_getCRC(dBuf);
		dLen = 3;
	}
	
	if (din->startTransaction(din->ioInterface))				din->coms_error |= MAX22_ERROR_COMS;
	if (din->transceiveBytes(din->ioInterface, 0, dBuf, dLen))	din->coms_error |= MAX22_ERROR_COMS;
	if (din->endTransaction(din->ioInterface))					din->coms_error |= MAX22_ERROR_COMS;
	
	if ( (din->crc_en == MAX22_CRCEN) && (max22_getCRC(dBuf) != (dBuf[2] & CRC_MASK)) )
	{
		din->coms_error |= MAX22_ERROR_CRC;
	}
	
	din->InputData = dBuf[0];
	din->wireBreak = dBuf[1];
}

enum MAX22_error max22_refreshInputs(max22_t *din)
{
	din->coms_error = MAX22_NOERROR;
	
	max22_writeReg(din, MAX22_R_NOP, 0);
	
	return din->coms_error;
}

uint8_t _FilterRegAddr(uint8_t n)
{
	uint8_t pos = 0, i = 1;
	
	while (!(i & n))
	{
		i = i << 1;
		++pos;
	}
	
	pos = (pos * 2) + MAX22_R_FILTER_IN1;
	
	return pos;
}

void max22_setInputFilter(max22_t *din, enum MAX22_INPUT filterInput, enum MAX22_FILTER filterSettings)
{
	uint8_t bitCheck = 0;
	din->coms_error = MAX22_NOERROR;
	
	// Check if the settings only have to be applied to one Input or multiple Inputs
	if ( (filterInput & (filterInput - 1)) == 0 )
	{
		// Apply only to one Input
		max22_writeReg(din, _FilterRegAddr(filterInput), filterSettings);
	}
	else
	{
		// Apply to multiple inputs		
		for (bitCheck = 0; bitCheck < 8; bitCheck++)
		{
			if (filterInput & (1<<bitCheck))
			{
				max22_writeReg(din, _FilterRegAddr(filterInput & (1<<bitCheck)), filterSettings);
			}
		}
	}
}

enum MAX22_FAULT max22_getFaults(max22_t *din)
{
	enum MAX22_FAULT faults = MAX22_NOFAULTS;
	din->coms_error = MAX22_NOERROR;
	
	faults = ((uint16_t)max22_readReg(din, MAX22_R_FAULT2) << 8) | max22_readReg(din, MAX22_R_FAULT1);
	
	return faults;
}