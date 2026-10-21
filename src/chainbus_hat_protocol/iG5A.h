#ifndef ZEPHYR_INCLUDE_IG5A_H 
#define ZEPHYR_INCLUDE_IG5A_H 

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

typedef uint8_t exception_code_t;

uint16_t CRC16(const uint8_t *data, uint16_t length);

// used for read_register() function
typedef enum
{
    REG_READ_NORMAL = 0,
    REG_READ_ONLY = 1
} iG5A_reg_access_t;

// Assumes that data ends with CRC16 split into low and high byte
bool verify_CRC16(const uint8_t *data, uint16_t length);

// Interface for a Modbus RTU slave device based on documentation
// https://www.modbustools.com/modbus.html
// Devices can share a serial interface, but concurrent communication
// will not work.

// ModbusRTU based interface for LS iG5A inverter based on documentation
// https://www.automation24.ir/media/uploads/files/products/LS_Inverter_SV-IG5A_Series_User_Manual.pdf
// Don't use polish manual, because the translation is trash.
// To communicate with the inverter you have to set following parameters:
// - Drv = 3
// - Frq = 7
// - I59 = 0
// - I60 = 1
// - I61 = 3
// - I62 = 0
// - I63 = 1

typedef struct iG5A iG5A;
struct iG5A
{
    enum Function
    {
        ReadHoldRegisters = 0x03,
        ReadInputRegisters = 0x04,
        WriteSingleRegister = 0x06,
        WriteMultipleRegisters = 0x10
    };
    enum Exception
    {
        NoException = 0x00,
        InvalidCRC = 0xFF,
        // iG5A specific
        //  Source: table on page 11-3
        IllegalFunction = 0x01,
        IllegalDataAddress = 0x02,
        IllegalDataValue = 0x03,
        SlaveDeviceBusy = 0x06,
        WriteDisabled = 0x14 // user defined RTFM
    };
    uint8_t slave_address;
};
bool set_frequency(const struct device* uartDev,iG5A *dev, float f);
bool start(const struct device* uartDev,iG5A *dev, bool reverse);
bool stop(const struct device* uartDev, iG5A *dev);
bool lock(const struct device* uartDev,iG5A *dev);
bool unlock(const struct device* uartDev, iG5A *dev);
bool is_locked(const struct device* uartDev, iG5A *dev);
bool is_running(const struct device* uartDev, iG5A *dev);
void iG5A_init(iG5A *device, uint8_t _slave_address);
exception_code_t read_register(const struct device* uartDev,iG5A *device, uint16_t address, uint16_t *input, iG5A_reg_access_t access);
exception_code_t write_register(const struct device* uartDev,iG5A *device, uint16_t address, uint16_t value);


#endif