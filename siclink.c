#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct Symbol
{
    char control_section[21];
    char name[7]; // 6 + 1 for the null terminator
    unsigned short int address;
    char length[13];
} Symbol;

Symbol ESTAB[100];
int symbolCount = 0;

int isDuplicateSymbol(const char* label)
{
    if (label == "")
    {
        return 0;
    }
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(ESTAB[i].name, label) == 0)
        {
            return 1; // Duplicate found
        }
    }
    return 0;
}

void addSymbol(const char* control_section, const char* name, unsigned short int address, const char* length)
{
    if (isDuplicateSymbol(name))
    {
        printf("Error: Pass 1: Duplicate symbol '%s'\n", name);
        exit(EXIT_FAILURE);
    }
    strcpy_s(ESTAB[symbolCount].control_section, sizeof(ESTAB[symbolCount].control_section), control_section);
    strcpy_s(ESTAB[symbolCount].name, sizeof(ESTAB[symbolCount].name), name);
    ESTAB[symbolCount].address = address;
    strcpy_s(ESTAB[symbolCount].length, sizeof(ESTAB[symbolCount].length), length);
    symbolCount++;
}

int getSymbolAddress(char* name)
{
    // Check for control section names 
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(ESTAB[i].control_section, name) == 0)
        {
            return ESTAB[i].address;
        }
    }

    // Check for symbol names
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(ESTAB[i].name, name) == 0)
        {
            return ESTAB[i].address;
        }
    }

    return -1;
}

void printSymbolTable(FILE* OutputFile, Symbol ESTAB[], int symbolCount)
{
    fprintf(OutputFile, "Symbol Table:\n");
    fprintf(OutputFile, "%-20s %-6s %-10s %-6s\n", "Control Section", "Name", "Address", "Length");

    for (int i = 0; i < symbolCount; i++)
    {
        fprintf(OutputFile, "%-20s %-6s 0x%-8X %-6s\n",
            ESTAB[i].control_section,
            ESTAB[i].name,
            ESTAB[i].address,
            ESTAB[i].length);
    }
}

typedef struct MemoryBuffer
{
    unsigned short int memory_address[5];
    short int contents[39];
} MemoryBuffer;

MemoryBuffer MEM[1024];
int memCount = 0;

void printMemoryBufferTable(FILE* OutputFile, MemoryBuffer MEM[], int memCount)
{
    fprintf(OutputFile, "Memory Buffer Table:\n");
    fprintf(OutputFile, "%-10s%-50s\n", "Address", "Contents");

    for (int i = 0; i < memCount; i++)
    {
        fprintf(OutputFile, "0x%04X    ", MEM[i].memory_address[0]);

        for (int j = 0; j < 16; j++)
        {
            if (MEM[i].contents[j] == -1)
            {
                fprintf(OutputFile, "..");
            }
            else
            {
                fprintf(OutputFile, "%02X", MEM[i].contents[j]);
            }

            if ((j + 1) % 4 == 0 && j < 15)
            {
                fprintf(OutputFile, "  ");
            }
        }
        fprintf(OutputFile, "\n");
    }
}

int getIndex(char* LOCATION)
{
    for (int i = 0; i < memCount; i++)
    {
        if (MEM[i].memory_address > MEM[0].memory_address + atoi(LOCATION))
        {
            return i - 1;
        }
    }
    return memCount - 1; // Return last index if not found
}

void processTextRecord(char* LINE, unsigned short int starting_address, int file_index)
{
    // Adds previous control section lengths
    unsigned int base_adjustment = 0;
    if (file_index > 0)
    {
        int found = 0;

        for (int j = 0; j < symbolCount; j++)
        {
            if (strlen(ESTAB[j].control_section) == 0)
                continue;

            unsigned int length = (unsigned int)strtol(ESTAB[j].length, NULL, 16);
            base_adjustment += length;

            found++;

            if (found == file_index)
                break;
        }
    }

    char hexAddress[7];
    strncpy_s(hexAddress, sizeof(hexAddress), LINE, 6);
    hexAddress[6] = '\0';

    unsigned int intAddress = (unsigned int)strtol(hexAddress, NULL, 16);
    unsigned int absoluteAddress = starting_address + base_adjustment + intAddress;

    char hexLength[3];
    strncpy_s(hexLength, sizeof(hexLength), LINE + 6, 2);
    hexLength[2] = '\0';

    unsigned int intLength = (unsigned int)strtol(hexLength, NULL, 16);

    char hexObjCode[61];
    strncpy_s(hexObjCode, sizeof(hexObjCode), LINE + 8, strlen(LINE + 8));
    hexObjCode[sizeof(hexObjCode) - 1] = '\0';

    size_t len = strlen(hexObjCode);
    while (len > 0 && (hexObjCode[len - 1] == ' ' || hexObjCode[len - 1] == '\n'))
    {
        hexObjCode[--len] = '\0';
    }

    for (unsigned int i = 0; i < intLength; i++)
    {
        char byteHex[3];
        strncpy_s(byteHex, sizeof(byteHex), &hexObjCode[i * 2], 2);
        byteHex[2] = '\0';

        unsigned char byte = (unsigned char)strtol(byteHex, NULL, 16);

        // Determine which memory buffer to use
        int memIndex = -1;
        for (int j = 0; j < memCount; j++)
        {
            if (MEM[j].memory_address[0] <= absoluteAddress && MEM[j].memory_address[0] + 16 > absoluteAddress)
            {
                memIndex = j;
                break;
            }
        }

        if (memIndex != -1)
        {
            int byteOffset = (absoluteAddress - MEM[memIndex].memory_address[0]) + i;

            // Handle overflow to the next buffer
            while (byteOffset >= 16)
            {
                memIndex++;
                byteOffset -= 16;
            }

            if (byteOffset >= 0 && byteOffset < 16)
            {
                MEM[memIndex].contents[byteOffset] = byte;
            }
        }
    }
}

void processModificationRecord(char* LINE, unsigned short int starting_address, int file_index)
{
    // Adds previous control section lengths
    unsigned int base_adjustment = 0;
    if (file_index > 0)
    {
        int found = 0;

        for (int j = 0; j < symbolCount; j++)
        {
            if (strlen(ESTAB[j].control_section) == 0)
                continue;

            unsigned int length = (unsigned int)strtol(ESTAB[j].length, NULL, 16);
            base_adjustment += length;

            found++;

            if (found == file_index)
                break;
        }
    }

    // Parse the modification record
    char LOCATION[7];
    strncpy_s(LOCATION, sizeof(LOCATION), LINE, 6);
    LOCATION[6] = '\0';

    char HALF_BYTES[3];
    strncpy_s(HALF_BYTES, sizeof(HALF_BYTES), LINE + 6, 2);
    HALF_BYTES[2] = '\0';

    char SIGN[2];
    SIGN[0] = LINE[8];
    SIGN[1] = '\0';

    char SYMBOL[100];
    strncpy_s(SYMBOL, sizeof(SYMBOL), LINE + 9, _TRUNCATE);

    // Convert location to absolute address
    unsigned int address = (unsigned int)strtol(LOCATION, NULL, 16) + starting_address + base_adjustment;
    unsigned int length = (unsigned int)strtol(HALF_BYTES, NULL, 16);

    // Get symbol address
    int symbolAddress = getSymbolAddress(SYMBOL);
    if (symbolAddress == -1)
    {
        printf("Error: Pass 2: Undefined symbol '%s'\n", SYMBOL);
        exit(EXIT_FAILURE);
    }

    // Find the correct memory buffer
    int memIndex = -1;
    int byteOffset = -1;
    int byteOffsetCopy = -1;
    for (int i = 0; i < memCount; i++)
    {
        if (MEM[i].memory_address[0] <= address && MEM[i].memory_address[0] + 16 > address)
        {
            memIndex = i;
            byteOffset = address - MEM[i].memory_address[0];
            byteOffsetCopy = byteOffset;
            break;
        }
    }

    int j = 0; char concatenatedHex[9]; concatenatedHex[0] = '\0'; char hexByte[3]; int newLine = 0;
    for (int i = (length / 2) + (length % 2); i > 0; i--)
    {
        if (byteOffset + j > 15)
        {
            memIndex++;
            byteOffset = 0;
            j = 0;
            newLine = 1;
        }
        int T = MEM[memIndex].contents[byteOffset + j]; // if byteOffset + j > 15, j = 0 and memIndex++;
        j++;
        sprintf_s(hexByte, sizeof(hexByte), "%02X", T);
        strcat_s(concatenatedHex, sizeof(concatenatedHex), hexByte);
    }

    int result = 0;
    if (strcmp(SIGN, "+") == 0)
    {
        result = (unsigned int)strtol(concatenatedHex, NULL, 16) + symbolAddress;
    }
    else if (strcmp(SIGN, "-") == 0)
    {
        result = (unsigned int)strtol(concatenatedHex, NULL, 16) - symbolAddress;
    }

    sprintf_s(concatenatedHex, sizeof(concatenatedHex), "%06X", result);
    j = 0; int k = 0;

    if (newLine == 1)
    {
        memIndex--;
        byteOffset = byteOffsetCopy;
    }
    if (strlen(concatenatedHex) >= 7)
    {
        k = strlen(concatenatedHex) - 6;
    }

    for (int i = (length / 2) + (length % 2); i > 0; i--)
    {
        if (byteOffset + j > 15)
        {
            memIndex++;
            byteOffset = 0;
            j = 0;
        }
        char hexPair[3];
        strncpy_s(hexPair, sizeof(hexPair), concatenatedHex + k, 2); 
        hexPair[2] = '\0';
        unsigned char byte = (unsigned char)strtol(hexPair, NULL, 16);
        MEM[memIndex].contents[byteOffset + j] = byte;
        j++;
        k += 2;
    }
}

//int main(int argc, char* argv[])
int main()
{
    /*if (argc < 4)  // Ensure at least 3 files are provided
    {
        printf("Usage: %s <file_path> <file_path> <file_path> [optional_additional_file_paths...]\n", argv[0]);
        return 1;
    }
    FILE* files[argc - 1];  // Array to store file pointers
    for (int i = 1; i < argc; i++)
    {
        files[i - 1] = fopen(argv[i], "r");

        if (files[i - 1] == NULL)
        {
            fprintf(stderr, "Error opening file %s: %s\n", argv[i], strerror(errno));

            return EXIT_FAILURE;
        }
    }*/

    FILE* files[3];
    fopen_s(&files[0], "PROGA.txt", "r");
    fopen_s(&files[1], "PROGB.txt", "r");
    fopen_s(&files[2], "PROGC.txt", "r");


    char line[256];
    char* context = NULL;
    unsigned short int starting_address = 0x4000;
    unsigned short int prev_starting_index = 0;

    // Pass 1
    for (int i = 0; i < 3; i++) //i < argc - 1
    {
        while (fgets(line, sizeof(line), files[i]))
        {
            if (line[0] == 'H')
            {
                char* control_section = strtok_s(line, " \n", &context);
                control_section++;
                char* length = strtok_s(NULL, " \n", &context);
                if (i == 0)
                {
                    addSymbol(control_section, "", starting_address, length);
                }
                else
                {
                    addSymbol(control_section, "", ESTAB[prev_starting_index].address + strtol(ESTAB[prev_starting_index].length, NULL, 16), length);
                    prev_starting_index = symbolCount - 1;
                }
            }
            else if (line[0] == 'D')
            {
                char* SYMBOL = strtok_s(line, " \n", &context);
                SYMBOL++;
                char* next = strtok_s(NULL, " \n", &context);
                char address[7];

                while (next != NULL)
                {
                    strncpy_s(address, sizeof(address), next, 6);
                    address[6] = '\0';
                    unsigned short int ADDRESS = (unsigned short int)strtol(address, NULL, 16); // Convert ADDRESS (hex string) to an unsigned short integer
                    addSymbol("", SYMBOL, ADDRESS + ESTAB[prev_starting_index].address, "");
                    SYMBOL = next + 6;
                    next = strtok_s(NULL, " \n", &context);
                }
            }
            else if (line[0] == 'R')
            {

                break;
            }
        }
    }

    fclose(files[0]);
    fclose(files[1]);
    fclose(files[2]);
    /*for (int i = 1; i < argc; i++)
    {
        fclose(files[i - 1]);
    }*/

    // Pass 2
    fopen_s(&files[0], "PROGA.txt", "r");
    fopen_s(&files[1], "PROGB.txt", "r");
    fopen_s(&files[2], "PROGC.txt", "r");

    FILE* OutputFile = fopen("OutputFile.txt", "w");
    printSymbolTable(OutputFile, ESTAB, symbolCount);
    char LOCATION[9]; char HALF_BYTES[3];  char SIGN[2]; char SYMBOL[100];

    for (int j = starting_address; j <= ESTAB[symbolCount - 1].address + 16; j += 16)
    {
        MEM[memCount].memory_address[0] = j;
        for (int k = 0; k < 16; k++)
        {
            MEM[memCount].contents[k] = -1;
        }
        memCount++;
    }

    for (int i = 0; i < 3; i++) //i < argc - 1
    {
        while (fgets(line, sizeof(line), files[i]))
        {
            if (line[0] == 'H')
            {

            }
            else if (line[0] == 'T')
            {
                char* LINE = strtok_s(line, " \n", &context);
                LINE++;

                processTextRecord(LINE, starting_address, i);
            }
            else if (line[0] == 'M')
            {
                char* LINE = strtok_s(line, " \n", &context);
                LINE++;

                processModificationRecord(LINE, starting_address, i);
            }
            else if (line[0] == 'E')
            {

                break;
            }
        }
    }

    printMemoryBufferTable(OutputFile, MEM, memCount);

    fclose(files[0]);
    fclose(files[1]);
    fclose(files[2]);
    /*for (int i = 1; i < argc; i++)
    {
        fclose(files[i - 1]);
    }*/
    fclose(OutputFile);
    return 0;
}
