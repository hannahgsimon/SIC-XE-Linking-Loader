#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

typedef struct Symbol
{
    char control_section[21];
    char name[7]; // 6 + 1 for the null terminator
    unsigned short int address;
    char length[13];
    char num[3];
} Symbol;

Symbol ESTAB[100];
int symbolCount = 0;

int isDuplicateSymbol(const char* label)
{
    if (label[0] == '\0')
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
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(ESTAB[i].name, name) == 0 || strcmp(ESTAB[i].control_section, name) == 0)
        {
            return ESTAB[i].address;
        }
    }
    return -1;
}

int getSymbolAddressFromR(char* number)
{
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(ESTAB[i].num, number) == 0)
        {
            return ESTAB[i].address;
        }
    }
    return -1;
}

int getSymbolIndex(char* name)
{
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(ESTAB[i].name, name) == 0 || strcmp(ESTAB[i].control_section, name) == 0 || strcmp(ESTAB[i].num, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void printSymbolTable(FILE* OutputFile, Symbol ESTAB[], int symbolCount, int RRecord)
{
    fprintf(OutputFile, "Symbol Table:\n");
    fprintf(OutputFile, "%-20s %-6s ", "Control Section", "Name");
    if (RRecord)
    {
        fprintf(OutputFile, "%-10s ", "Number");
    }
    fprintf(OutputFile, "%-10s %-6s\n", "Address", "Length");

    for (int i = 0; i < symbolCount; i++)
    {
        char* trimmedLength = ESTAB[i].length;
        while (*trimmedLength == '0' && *(trimmedLength + 1) != '\0')
        {
            trimmedLength++;
        }
        char prefixedLength[15] = "";
        if (*trimmedLength != '\0')
        {
            snprintf(prefixedLength, sizeof(prefixedLength), "0x%s", trimmedLength);
        }

        if (RRecord)
        {
            fprintf(OutputFile, "%-20s %-6s %-10s 0x%-8X %-6s\n",
                ESTAB[i].control_section,
                ESTAB[i].name,
                ESTAB[i].num,
                ESTAB[i].address,
                prefixedLength);
        }
        else
        {
            fprintf(OutputFile, "%-20s %-6s 0x%-8X %-6s\n",
                ESTAB[i].control_section,
                ESTAB[i].name,
                ESTAB[i].address,
                prefixedLength);
        }
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

void processTextRecord(char* LINE, unsigned short int PROGADDR, int file_index)
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
    unsigned int absoluteAddress = PROGADDR + base_adjustment + intAddress;

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

void processModificationRecord(char* LINE, unsigned short int PROGADDR, int file_index, int RRecord)
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
    unsigned int address = (unsigned int)strtol(LOCATION, NULL, 16) + PROGADDR + base_adjustment;
    unsigned int length = (unsigned int)strtol(HALF_BYTES, NULL, 16);

    // Get symbol address
    int symbolAddress;
    if (!RRecord)
    {
        symbolAddress = getSymbolAddress(SYMBOL);
    }
    else
    {
        symbolAddress = getSymbolAddressFromR(SYMBOL);
    }
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

int main(int argc, char* argv[])
{
    if (argc < 5)  // Ensure at least 3 files are provided
    {
        printf("Usage: %s <file_path> <file_path> <file_path> [optional_additional_file_paths...] <starting_address_in_hex>\n", argv[0]);
        return 1;
    }
    FILE** files = malloc((argc - 2) * sizeof(FILE*));  // Array to store file pointers
    for (int i = 1; i < argc - 1; i++)
    {
        if (fopen_s(&files[i - 1], argv[i], "r") != 0)
        {
            char errorMsg[256];
            strerror_s(errorMsg, sizeof(errorMsg), errno);
            fprintf(stderr, "Error opening file %s: %s\n", argv[i], errorMsg);
            return EXIT_FAILURE;
        }
    }

    unsigned short int PROGADDR = 0; char* endptr = NULL;

    PROGADDR = (unsigned short int) strtol(argv[argc - 1], &endptr, 16);
    if (*endptr != '\0')  // Check if conversion was successful
    {
        fprintf(stderr, "Invalid starting address format: %s\n", argv[argc - 1]);
        return EXIT_FAILURE;
    }

    char line[256];
    char* context = NULL;
    unsigned short int prev_PROGADDR = 0;

    // Pass 1
    for (int i = 0; i < argc - 1; i++)
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
                    addSymbol(control_section, "", PROGADDR, length);
                }
                else
                {
                    addSymbol(control_section, "", ESTAB[prev_PROGADDR].address + strtol(ESTAB[prev_PROGADDR].length, NULL, 16), length);
                    prev_PROGADDR = symbolCount - 1;
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
                    addSymbol("", SYMBOL, ADDRESS + ESTAB[prev_PROGADDR].address, "");
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

    for (int i = 1; i < argc - 1; i++)
    {
        fclose(files[i - 1]);
    }

    // Pass 2
    for (int i = 1; i < argc - 1; i++)
    {
        if (fopen_s(&files[i - 1], argv[i], "r") != 0)
        {
            char errorMsg[256];
            strerror_s(errorMsg, sizeof(errorMsg), errno);
            fprintf(stderr, "Error opening file %s: %s\n", argv[i], errorMsg);
            return EXIT_FAILURE;
        }
    }

    FILE* OutputFile = fopen("OutputFile.txt", "w");
    char LOCATION[9]; char HALF_BYTES[3];  char SIGN[2]; char SYMBOL[100];
    int RRecord = 0; int j = 0;

    for (int j = PROGADDR; j <= ESTAB[symbolCount - 1].address + 16; j += 16)
    {
        MEM[memCount].memory_address[0] = j;
        for (int k = 0; k < 16; k++)
        {
            MEM[memCount].contents[k] = -1;
        }
        memCount++;
    }
    for (int i = 0; i < argc - 1; i++)
    {
        while (fgets(line, sizeof(line), files[i]))
        {
            if (line[0] == 'H')
            {
                strcpy_s(ESTAB[j].num, sizeof(ESTAB[j].num), "");
                char* control_section = strtok_s(line, " \n", &context);
                control_section++;
                j = getSymbolIndex(control_section);
            }
            else if (line[0] == 'R')
            {
                char* LINE = strtok_s(line, " \n", &context);
                LINE++;
                if (!isdigit(LINE[0]))
                {
                    continue;
                }
                RRecord = 1;
                strcpy_s(ESTAB[j].num, sizeof(ESTAB[j].num), "01");
                while (LINE != NULL)
                {
                    char NUMBER[10] = { 0 };
                    char SYMBOL[7] = { 0 };
                    int i = 0;
                    while (LINE[i] != '\0' && isdigit(LINE[i]))
                    {
                        NUMBER[i] = LINE[i];
                        i++;
                    }
                    NUMBER[i] = '\0';
                    strcpy_s(SYMBOL, sizeof(SYMBOL), LINE + i);
                    i = getSymbolIndex(NUMBER);
                    if (i != -1)
                    {
                        strcpy_s(ESTAB[i].num, sizeof(ESTAB[i].num), "");
                    }
                    i = getSymbolIndex(SYMBOL);
                    strcpy_s(ESTAB[i].num, sizeof(ESTAB[i].num), NUMBER);
                    LINE = strtok_s(NULL, " \n", &context);
                }
            }
            else if (line[0] == 'T')
            {
                char* LINE = strtok_s(line, " \n", &context);
                LINE++;
                processTextRecord(LINE, PROGADDR, i);
            }
            else if (line[0] == 'M')
            {
                char* LINE = strtok_s(line, " \n", &context);
                LINE++;

                processModificationRecord(LINE, PROGADDR, i, RRecord);
            }
            else if (line[0] == 'E')
            {

                break;
            }
        }
    }
    printSymbolTable(OutputFile, ESTAB, symbolCount, RRecord);
    printMemoryBufferTable(OutputFile, MEM, memCount);

    for (int i = 1; i < argc - 1; i++)
    {
        fclose(files[i - 1]);
    }
    fclose(OutputFile);
    printf("Output file created: OutputFile.txt\n");
    free(files);
    return 0;
}
