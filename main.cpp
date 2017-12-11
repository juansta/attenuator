#include <mcp2210.h>
#include <QCoreApplication>
#include <QDebug>

#define OUTPUT_LED 0
#define OUTPUT_CS 1
#define INPUT_D0 2
#define INPUT_D1 3
#define INPUT_D2 4
#define INPUT_D3 5
#define INPUT_D4 6
#define INPUT_D5 7
#define INPUT_PS 8
#define INPUT_TOTAL 9

#define BIT0 0
#define BIT1 1
#define BIT2 2
#define BIT3 3
#define BIT4 4
#define BIT5 5
#define VALID_BITS 0x3f

ChipStatusDef get_status(hid_device *handle)
{
    ChipStatusDef def = GetChipStatus(handle);

    qDebug() << "SPIBusCurrentOwner -- " << def.SPIBusCurrentOwner;

    return def;
}

ChipSettingsDef configure_chip(hid_device *handle)
{
    ChipSettingsDef def = GetChipSettings(handle);

    def.RemoteWakeUpEnabled               = 0;
    def.DedicatedFunctionInterruptPinMode = 0;
    def.SPIBusReleaseMode                 = 0;
    def.NVRamChipParamAccessControl       = 0;

    SetChipSettings(handle, def);

    return def;
}
SPITransferSettingsDef configure_spi(hid_device *handle)
{
    SPITransferSettingsDef def = GetSPITransferSettings(handle);

    def.BitRate                 = 1500;
    def.IdleChipSelectValue     = (1 << OUTPUT_CS);
    def.ActiveChipSelectValue   = (0 << OUTPUT_CS);
    def.CSToDataDelay           = 0;
    def.LastDataByteToCSDelay   = 0;
    def.SubsequentDataByteDelay = 0;
    def.BytesPerSPITransfer     = 1;
    def.SPIMode                 = 3;

    SetSPITransferSettings(handle, def);

    return def;
}

GPPinDef configure_gpio(hid_device *handle)
{
    GPPinDef def = GetGPIOPinDirection(handle);

    def.GP[OUTPUT_LED].GPIODirection  = GPIO_DIRECTION_OUTPUT;
    def.GP[OUTPUT_LED].PinDesignation = GP_PIN_DESIGNATION_GPIO;

    def.GP[OUTPUT_CS].GPIODirection  = GPIO_DIRECTION_OUTPUT;
    def.GP[OUTPUT_CS].PinDesignation = GP_PIN_DESIGNATION_CS;

    for (int i = INPUT_D0; i < INPUT_TOTAL; i++)
    {
        def.GP[i].GPIODirection  = GPIO_DIRECTION_INPUT;
        def.GP[i].PinDesignation = GP_PIN_DESIGNATION_GPIO;
    }

    SetGPIOPinDirection(handle, def);

    return def;
}

SPIDataTransferStatusDef send_spi(hid_device *handle, byte &attenuation)
{
    SPIDataTransferStatusDef transfer =
        SPIDataTransfer(handle, &attenuation, 1);
    SPIDataTransfer(handle, &attenuation, 1);
    SPIDataTransfer(handle, &attenuation, 1);
    SPIDataTransfer(handle, &attenuation, 1);

    return transfer;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    hid_device *handle = InitMCP2210();

    if (handle)
    {
        qDebug() << "Found Device";

        get_status(handle);

        GPPinDef               gpio_def = configure_gpio(handle);
        SPITransferSettingsDef spi_def  = configure_spi(handle);

        int  r             = 0;
        int  timeout       = 64 * 5;
        int  dbAttenuation = 0;
        byte attenuation   = VALID_BITS;

        GPPinDef                 inputs       = GetGPIOPinValue(handle);
        SPIDataTransferStatusDef spi_transfer = send_spi(handle, attenuation);

        while (!r && timeout-- && !inputs.ErrorCode && !spi_transfer.ErrorCode)
        {
            gpio_def.GP[OUTPUT_LED].GPIOOutput =
                !gpio_def.GP[OUTPUT_LED].GPIOOutput;

            r = SetGPIOPinVal(handle, gpio_def);

            qDebug() << "INPUT -- " << inputs.GP[INPUT_D5].GPIOOutput
                     << inputs.GP[INPUT_D4].GPIOOutput
                     << inputs.GP[INPUT_D3].GPIOOutput
                     << inputs.GP[INPUT_D2].GPIOOutput
                     << inputs.GP[INPUT_D1].GPIOOutput
                     << inputs.GP[INPUT_D0].GPIOOutput
                     << inputs.GP[INPUT_PS].GPIOOutput << inputs.ErrorCode
                     << spi_transfer.ErrorCode << attenuation << dbAttenuation;

            sleep(5);

            inputs       = GetGPIOPinValue(handle);
            spi_transfer = send_spi(handle, attenuation);

            dbAttenuation = (dbAttenuation + 1) % 64;
            attenuation   = (~dbAttenuation & VALID_BITS);
        }

        ReleaseMCP2210(handle);
    }

    return 0;
}
