#include "getCommandLineFromSerialPort.h"

bool getCommandLineFromSerialPort(char *commandLine)
{
    static uint8_t charsRead = 0; // note: COMAND_BUFFER_LENGTH must be less than 255 chars long
    // read asynchronously until full command input
    while (Serial.available())
    {
        char c = Serial.read();
        switch (c)
        {
        case CR: // likely have full command in buffer now, commands are terminated by CR and/or LS
        case LF:
            commandLine[charsRead] = NULLCHAR; // null terminate our command char array
            if (charsRead > 0)
            {
                charsRead = 0; // charsRead is static, so have to reset
                // Serial.println(commandLine);
                return true;
            }
            break;
        case BS: // handle backspace in input: put a space in last char
            if (charsRead > 0)
            { // and adjust commandLine and charsRead
                commandLine[--charsRead] = NULLCHAR;
                Serial << byte(BS) << byte(SPACE) << byte(BS); // no idea how this works, found it on the Internet
            }
            break;
        default:
            c = toupper(c);
            if (charsRead < COMMAND_BUFFER_LENGTH)
            {
                commandLine[charsRead++] = c;
            }
            commandLine[charsRead] = NULLCHAR; // just in case
            break;
        }
    }
    return false;
}
