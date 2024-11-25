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

Symbol symbolTable[100];
int symbolCount = 0;

int isDuplicateSymbol(const char* label)
{
    if (label == "")
    {
        return 0;
    }
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(symbolTable[i].name, label) == 0)
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
    strcpy_s(symbolTable[symbolCount].control_section, sizeof(symbolTable[symbolCount].control_section), control_section);
    strcpy_s(symbolTable[symbolCount].name, sizeof(symbolTable[symbolCount].name), name);
    if (control_section == "")
    {
        symbolTable[symbolCount].address = symbolTable[symbolCount - 1].address + address;
    }
    else
    {
        symbolTable[symbolCount].address = address;
    }
    strcpy_s(symbolTable[symbolCount].length, sizeof(symbolTable[symbolCount].length), length);
    symbolCount++;
}

int getSymbolAddress(char* name)
{
    if (strlen(name) >= 2 && name[strlen(name) - 2] == ',' && name[strlen(name) - 1] == 'X')
    {
        name[strlen(name) - 2] = '\0';
    }
    else if (name[0] == '@' || name[0] == '#')
    {
        memmove(name, name + 1, strlen(name));
    }
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(symbolTable[i].name, name) == 0)
        {
            return symbolTable[i].address; // Return the address if found
        }
    }
    return NULL;
}

void printSymbolTable(Symbol symbolTable[], int symbolCount)
{
    printf("Symbol Table:\n");
    printf("%-20s %-6s %-10s %-6s\n", "Control Section", "Name", "Address", "Length");

    for (int i = 0; i < symbolCount; i++)
    {
        printf("%-20s %-6s 0x%-8X %-6s\n",
            symbolTable[i].control_section,
            symbolTable[i].name,
            symbolTable[i].address,
            symbolTable[i].length);
    }
}

typedef struct MemoryBuffer
{
    unsigned short int memory_address[5];
    unsigned short int contents1[9];
    unsigned short int contents2[9];
    unsigned short int contents3[9];
    unsigned short int contents4[9];
} MemoryBuffer;

MemoryBuffer MEM[100];
int memCount = 0;

void addMem(unsigned short int address,
    const unsigned short int* contents1,
    const unsigned short int* contents2,
    const unsigned short int* contents3,
    const unsigned short int* contents4)
{
    // Set the memory address
    MEM[memCount].memory_address[0] = address;
    for (int i = 1; i < 5; i++)
    {
        MEM[memCount].memory_address[i] = 0; // Initialize remaining memory addresses to 0
    }

    // Copy contents into respective arrays
    if (contents1 != NULL)
    {
        memcpy(MEM[memCount].contents1, contents1, sizeof(MEM[memCount].contents1));
    }
    else
    {
        memset(MEM[memCount].contents1, 0, sizeof(MEM[memCount].contents1)); // Set to 0 if contents1 is NULL
    }

    if (contents2 != NULL)
    {
        memcpy(MEM[memCount].contents2, contents2, sizeof(MEM[memCount].contents2));
    }
    else
    {
        memset(MEM[memCount].contents2, 0, sizeof(MEM[memCount].contents2));
    }

    if (contents3 != NULL)
    {
        memcpy(MEM[memCount].contents3, contents3, sizeof(MEM[memCount].contents3));
    }
    else
    {
        memset(MEM[memCount].contents3, 0, sizeof(MEM[memCount].contents3));
    }

    if (contents4 != NULL)
    {
        memcpy(MEM[memCount].contents4, contents4, sizeof(MEM[memCount].contents4));
    }
    else
    {
        memset(MEM[memCount].contents4, 0, sizeof(MEM[memCount].contents4));
    }

    memCount++;
}

void printMemoryTable()
{
    if (memCount == 0)
    {
        printf("Memory table is empty.\n");
        return;
    }

    printf("Memory Table:\n");
    printf("-------------------------------------------------------------------\n");
    printf("| Address | Contents1         | Contents2         | Contents3         | Contents4         |\n");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < memCount; i++)
    {
        printf("| %7d | ", MEM[i].memory_address[0]);

        // Print each contents array in the same row
        for (int j = 0; j < 9; j++)
        {
            printf("%2d ", MEM[i].contents1[j]);
        }
        printf("| ");

        for (int j = 0; j < 9; j++)
        {
            printf("%2d ", MEM[i].contents2[j]);
        }
        printf("| ");

        for (int j = 0; j < 9; j++)
        {
            printf("%2d ", MEM[i].contents3[j]);
        }
        printf("| ");

        for (int j = 0; j < 9; j++)
        {
            printf("%2d ", MEM[i].contents4[j]);
        }
        printf("|\n");
    }

    printf("-------------------------------------------------------------------\n");
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
    unsigned short int prev_starting_address = starting_address;


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
                    unsigned short int length_int = (unsigned short int)strtol(length, NULL, 16);   // Convert length (hex string) to an unsigned short integer
                    int new_address = prev_starting_address + length_int;
                    addSymbol(control_section, "", new_address, length);
                    prev_starting_address = new_address;
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
                    addSymbol("", SYMBOL, ADDRESS, "");
                    SYMBOL = next + 6;
                    next = strtok_s(NULL, " \n", &context);
                }

            }
            else if (line[0] == 'E')
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
    fopen_s(&files[0], "PROGA.txt", "r");
    fopen_s(&files[1], "PROGB.txt", "r");
    fopen_s(&files[2], "PROGC.txt", "r");

    printSymbolTable(symbolTable, symbolCount);
    FILE* OutputFile = fopen("OutputFile.txt", "w");
    char LOCATION[9]; char HALF_BYTES[3];  char SIGN[2]; char SYMBOL[100];

    addMem(3, 4, 4, 4, 4);
    printMemoryTable();

    for (int i = 0; i < 3; i++) //i < argc - 1
    {
        while (fgets(line, sizeof(line), files[i]))
        {
            if (line[0] == 'H')
            {

            }
            else if (line[0] == 'T')
            {

            }
            else if (line[0] == 'M')
            {
                char* LINE = strtok_s(line, " \n", &context);
                LINE++;
                strncpy_s(LOCATION, sizeof(LOCATION), LINE, 6);
                LOCATION[6] = '\0';
                strncpy_s(HALF_BYTES, sizeof(HALF_BYTES), LINE + 6, 2);
                HALF_BYTES[2] = '\0';
                SIGN[0] = LINE[8];
                SIGN[1] = '\0';
                strncpy_s(SYMBOL, sizeof(SYMBOL), LINE + 9, _TRUNCATE);
                //printf("LOCATION: '%s'\n", LOCATION);
                //printf("HALF_BYTES: '%s'\n", HALF_BYTES);
                //printf("SIGN: '%s'\n", SIGN);
                //printf("SYMBOL: '%s'\n\n", SYMBOL);
            }
            else if (line[0] == 'E')
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
    fclose(OutputFile);
    return 0;
}
