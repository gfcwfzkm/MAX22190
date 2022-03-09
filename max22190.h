/*
 * max22190.h
 *
 * Created: 19.01.2021 07:18:15
 *  Author: gfcwfzkm
 */ 

#ifndef MAX22190_H_
#define MAX22190_H_

#include <inttypes.h>

#define MAX22_C_WRITE		0x80	// Data Write Bit (MSB)
#define MAX22_C_READ		0x00	// Data Read Bit (MSB)

/* Registers */
#define MAX22_R_WIREBREAK	0x00	// Clear-on-Read
#define MAX22_R_DIGITALIN	0x02	// Read only
#define MAX22_R_FAULT1		0x04	// Mixed
#define MAX22_R_FAULT1_CRC		0x80	// CRC Error, cleared by a correct SPI communication
#define MAX22_R_FAULT1_POR		0x40	// Power-On-Reset occurred, register map resetted
#define MAX22_R_FAULT1_FLT2		0x20	// Fault2 Reg has bits active (error)
#define MAX22_R_FAULT1_ALRM2	0x10	// Overtemperature alarm 1
#define MAX22_R_FAULT1_ALRM1	0x08	// Overtemperature alarm 2
#define MAX22_R_FAULT1_24VL		0x04	// VDD24 low voltage alarm
#define MAX22_R_FAULT1_24VM		0x02	// VDD24 voltage missing alarm
#define MAX22_R_FAULT1_WBG		0x01	// Wire-break group error detected
#define MAX22_R_FILTER_IN1	0x06	// RW
#define MAX22_R_FILTER_IN2	0x08	// RW
#define MAX22_R_FILTER_IN3	0x0A	// RW
#define MAX22_R_FILTER_IN4	0x0C	// RW
#define MAX22_R_FILTER_IN5	0x0E	// RW
#define MAX22_R_FILTER_IN6	0x10	// RW
#define MAX22_R_FILTER_IN7	0x12	// RW
#define MAX22_R_FILTER_IN8	0x14	// RW
#define MAX22_R_CONFIG		0x18	// RW
#define MAX22_R_INEN		0x1A	// RW
#define MAX22_R_FAULT2		0x1C	// COR
#define MAX22_R_FAULT2_FAULT8CK	0x20	// SPI clock pulses not equal a multiple of 8, command rejected
#define MAX22_R_FAULT2_OTSHDN	0x10	// Overtemperature Shutdown (safe temp exceeded)
#define MAX22_R_FUALT2_RFDIO	0x08	// Open Condition on REFDI pin
#define MAX22_R_FAULT2_RFDIS	0x04	// Short condition on REFDI pin
#define MAX22_R_FAULT2_RFWBO	0x02	// Open Condition on RFWBO pin
#define MAX22_R_FAULT2_RFWBS	0x01	// Short condition on RFWBO pin
#define MAX22_R_FAULT2EN	0x1E	// RW	Config which error is asserted to the FAULT Pin
#define MAX22_R_GPO			0x22	// RW
#define MAX22_R_FAULT1EN	0x24	// RW	Config which error is asserted to the FAULT Pin
#define MAX22_R_NOP			0x26	// NA

// decides wether a 16bit or 24bit packet is sent / received
enum MAX22_CRCEN{
	MAX22_NOCRC = 0,
	MAX22_CRCEN = 1
};

enum MAX22_CFG{
	MAX22_CFG_24VF	= 0x10,
	MAX22_CFG_CLRF	= 0x08,
	MAX22_CFG_REFDI	= 0x01
};

enum MAX22_FILTER{
	MAX22_FILTER_WIREBREAK_EN	= 0x10,
	MAX22_FILTER_BYPASS		= 0x08,
	MAX22_FILTER_DELAY_20MS	= 0x07,
	MAX22_FILTER_DELAY_12_8MS	= 0x06,
	MAX22_FILTER_DELAY_3_2MS	= 0x05,
	MAX22_FILTER_DELAY_1_6MS	= 0x04,
	MAX22_FILTER_DELAY_800US	= 0x03,
	MAX22_FILTER_DELAY_400US	= 0x02,
	MAX22_FILTER_DELAY_100US	= 0x01,
	MAX22_FILTER_DELAY_50US	= 0x00
};

enum MAX22_INPUT{
	MAX22_IN1	= 0x01,
	MAX22_IN2	= 0x02,
	MAX22_IN3	= 0x04,
	MAX22_IN4	= 0x08,
	MAX22_IN5	= 0x10,
	MAX22_IN6	= 0x20,
	MAX22_IN7	= 0x40,
	MAX22_IN8	= 0x80
};

enum MAX22_FAULT{
	MAX22_NOFAULTS	= 0x0000,
	MAX22_WIREBREAK	= 0x0001,
	MAX22_24VM		= 0x0002,
	MAX22_24VL		= 0x0004,
	MAX22_ALARMT1	= 0x0008,
	MAX22_ALARMT2	= 0x0010,
	MAX22_FAULT2	= 0x0020,
	MAX22_POR		= 0x0040,
	MAX22_CRC		= 0x0080,
	MAX22_RFWBS		= 0x0100,
	MAX22_RFBWO		= 0x0200,
	MAX22_RFDIS		= 0x0400,
	MAX22_RFDIO		= 0x0800,
	MAX22_OTSHDN	= 0x1000,
	MAX22_FAULT8CK	= 0x2000
};

enum MAX22_error{
	MAX22_NOERROR		= 0x00,
	MAX22_ERROR_COMS	= 0x01,
	MAX22_ERROR_CRC		= 0x02,	// only used with 24bit transmission size, CRC Error
	MAX22_ERROR_24VL	= 0x04,	// only used with 24bit transmission size
	MAX22_ERROR_24VM	= 0x08,	// only used with 24bit transmission size
	MAX22_ERROR_WB		= 0x10	// only used with 24bit transmission size, Wirebreak error
};

typedef struct
{
	enum MAX22_CRCEN crc_en:1;			// CRC is enabled or not
	enum MAX22_error coms_error:5;			// Errors
	uint8_t InputData;					// The digital Inputs of the chip, refreshed with every transaction
	uint8_t wireBreak;					// WireBreak Data, refreshed with every write
	void *ioInterface;					// Pointer to the IO/Peripheral Interface library
	// Any return value by the IO interface functions have to return zero when successful or
	// non-zero when not successful.
	uint8_t (*startTransaction)(void*);	// Prepare the IO/Peripheral Interface for a transaction
	uint8_t (*transceiveBytes)(void*,	// Send and receive Bytes from the buffer (SPI only)
						uint8_t,		// Address of the chip (8-Bit Address Format!) (ignored if zero),
						uint8_t*,		// Pointer to send buffer,
						uint16_t);		// Amount of bytes to send
	uint8_t (*endTransaction)(void*);	// Finish the transaction / Release IO/Peripheral
} max22_t;

void max22_initStruct(max22_t *din, void *ioComs,
				uint8_t (*startTransaction)(void*),
				uint8_t (*transceiveBytes)(void*,uint8_t,uint8_t*,uint16_t),
				uint8_t (*endTransaction)(void*));

// Only SPI Mode 0 tested
enum MAX22_error max22_init(max22_t *din, enum MAX22_CRCEN crcen);

uint8_t max22_readReg(max22_t *din, uint8_t regAddr);

void max22_writeReg(max22_t *din, uint8_t regAddr, uint8_t val);

enum MAX22_error max22_refreshInputs(max22_t *din);

void max22_setInputFilter(max22_t *din, enum MAX22_INPUT filterInput, enum MAX22_FILTER filterSettings);

enum MAX22_FAULT max22_getFaults(max22_t *din);

#endif /* MAX22190_H_ */