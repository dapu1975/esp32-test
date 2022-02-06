// getCommandLineFromSerialPort.h
#ifndef getCommandLineFromSerialPort_H
#define getCommandLineFromSerialPort_H

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

#define CR '\r'
#define LF '\n'
#define BS '\b'
#define NULLCHAR '\0'
#define SPACE ' '

#define COMMAND_BUFFER_LENGTH 25 // length of serial buffer for incoming commands

bool getCommandLineFromSerialPort(char *commandLine);

#endif // getCommandLineFromSerialPort_H
