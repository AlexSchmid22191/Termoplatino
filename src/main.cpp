//
// Created by alex on 23/04/19.
//

#include <Adafruit_MAX31865.h>
#include <Arduino.h>
#include "SPI.h"

//Reference and nominal resistance
#define RREF      430.0
#define RNOMINAL  100.0

// Pin definitions
const byte RTD_CS = 10;

// RTD readout chip
Adafruit_MAX31865 MAX = Adafruit_MAX31865(RTD_CS);

//Measured variables
float temp_filtered = 0;
byte fault = 0;

//Smoothing factor for exponential moving avergae filter
float f_smooth = 0.1;

void listen_to_serial();
void report();
void report_temperature(float temperature);
void report_error(byte errorcode);

void setup()
{
    MAX.begin(MAX31865_4WIRE);
    Serial.begin(115200);
}

void loop()
{
    //Read temperature from MAX chip (also performs fault detection)
    float temp_raw = MAX.temperature(RNOMINAL, RREF);

    //Read fault data from chip
    fault = MAX.readFault();

    //Apply EMA filter when no fault is detected
    if(!fault) temp_filtered = temp_filtered * (1 - f_smooth) + temp_raw * f_smooth;

    //Check for serial communication
    listen_to_serial();
}

void listen_to_serial()
{
    char commandBuffer[51];
    char *commandPointers[2];

    //Check if a serial communication is requested
    while((bool)Serial.available())
    {
        //Read bytes until the start character (:) is encounterd
        int x = Serial.read();
        if(x == ':')
        {
            //Read up to 50 bytes into the command buffer until a line feed is encountered
            memset(commandBuffer, 0, 50);
            Serial.readBytesUntil(0x0A, commandBuffer, 50);

            //Slice the command string at every delimiter (blank space) and store pointers in command pointers
            commandPointers[0] = strtok(commandBuffer, " ");
            byte i = 0;
            while(commandPointers[i] != nullptr)
            {
                i++;
                commandPointers[i] = strtok(nullptr, ",");
            }

            //Check if a data read is requested
            if(!(bool)strncmp(commandPointers[0], "read?", 5))
            {
                report();
            }
        }
    }
}


void report_temperature(float temperature)
{
    char printString[15];
    dtostre(temperature, printString, 7, DTOSTR_UPPERCASE | DTOSTR_PLUS_SIGN);
    Serial.println(printString);
}

void report_error(byte errorcode)
{
    Serial.print("ER");
    if(errorcode & MAX31865_FAULT_HIGHTHRESH)
    {
        Serial.print(",HIGHTHRESH");
    }
    if(errorcode & MAX31865_FAULT_LOWTHRESH)
    {
        Serial.print(",LOWTHRESH");
    }
    if(errorcode & MAX31865_FAULT_REFINLOW)
    {
        Serial.print(",REFINLOW");
    }
    if(errorcode & MAX31865_FAULT_REFINHIGH)
    {
        Serial.print(",REFINHIGH");
    }
    if(errorcode & MAX31865_FAULT_RTDINLOW)
    {
        Serial.print(",RTDINLOW");
    }
    if(errorcode & MAX31865_FAULT_OVUV)
    {
        Serial.print(",OVERUNDER");
    }
    Serial.println();
}

void report()
{
    if(!fault) report_temperature(temp_filtered);
    else report_error(fault);
}
