// HExadecimal RedactOr VersIoN Alpha (HeRoVinA)
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LINES_PER_SCREEN 16

#define RED "\033[31m"
#define LRED "\033[91m"
#define MAGENTA "\033[35m"
#define LMAGENTA "\033[95m"
#define LBLACK "\033[90m"
#define RESET "\033[0m"
#define YELLOW "\033[33m"
#define LYELLOW "\033[93m"

int getch() {
    struct termios oldt, newt;
    int ch;

    tcgetattr(STDIN_FILENO, &oldt);         
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);       
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); 
    return ch;
}

void renderLine(unsigned int lineAddress, unsigned int selectedAddress, int editMode, unsigned char* buffer, unsigned int fileSize) {
    printf("[%s%08X%s]  ", MAGENTA, lineAddress, RESET);

    char* selectionColors[] = {LMAGENTA, RED, YELLOW, LRED, LYELLOW};
    char* selectionColor = selectionColors[editMode];

    for (int offset = 0; offset < 16; offset++) {
        if (lineAddress + offset < fileSize) {
            unsigned char symbol = buffer[lineAddress + offset];
            if (lineAddress + offset == selectedAddress) printf(selectionColor);
            printf("%02X%s ", symbol, RESET);
        } else {
            printf("   ");
        }
    }

    printf(" | ");

    for (int offset = 0; offset < 16; offset++) {
        unsigned char symbol = buffer[lineAddress + offset];
        if (lineAddress + offset < fileSize && symbol >= 32 && symbol <= 126) {
            if (lineAddress + offset == selectedAddress) printf(selectionColor);
            printf("%c%s", symbol, RESET);
        } else {
            printf("%s.%s", LBLACK, RESET);
        }
    }
    printf("\n");
}

void renderScreen (unsigned int screenAddress, unsigned int selectedAddress, int editMode, unsigned char* buffer, unsigned int fileSize, char* filename) {
    system("clear");

    printf("            %s0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F%s\n", MAGENTA, RESET);

    unsigned int lineAddress;
    for (int line = 0; line < LINES_PER_SCREEN && 16 * line + screenAddress < fileSize; line++) {
        lineAddress = screenAddress + 16 * line;
        renderLine(lineAddress, selectedAddress, editMode, buffer, fileSize);
    }

    printf("\n[%s%08X%s]", LMAGENTA, selectedAddress, RESET);
    printf("  (%s%s%s)", MAGENTA, filename, RESET);
    printf("  %d bytes", fileSize);
    printf(" (%s00000000%s - %s%08X%s)", LMAGENTA, RESET, LMAGENTA, fileSize - 1, RESET);
    printf("\n");
}

void waitForEnter() {
    while (1) {
        if (getch() == 13) break; // if getch() == enter
    }
}

int inputByte() { // normally returns values 0-255 and has -1 as an error status code
    unsigned char value = 0;
    for (int i = 0; i < 2; i++) {
        char input = getch();
        value *= 16;
        if ('0' <= input && input <= '9') value += (input - '0' + 0);
        else if ('a' <= input && input <= 'f') value += (input - 'a' + 10);
        else if (input == 27) return -27;
        else return -1;
    }
    printf("\n");
    return value;
}

long long inputInt() { // normally returns values 0 - 2^32-1 and has -1 as an error status code
    unsigned int value = 0;
    while (1) {
        char input = getch();
        if (input == 13) break; // enter
        value *= 16;
        char output;
        if ('a' <= input && input <= 'f') output = input - 'a' + 'A';
        else output = input;
        printf("%s%c%s", LMAGENTA, output, RESET);
        if ('0' <= input && input <= '9') value += (input - '0' + 0);
        else if ('a' <= input && input <= 'f') value += (input - 'a' + 10);
        else if ('A' <= input && input <= 'F') value += (input - 'A' + 10);
        else return -1;
    }
    printf("\n");
    return value;
}

void printHelp() {
    printf("wasd - navigating\n");
    printf("e - edit byte (enter byte in hex)\n");
    printf("c - edit byte (enter byte as character)\n");
    printf("E - enter sequence of bytes in hexadecimal (quit with 'esc')\n");
    printf("W - enter sequence of bytes as characters (quit with 'esc')\n");
    printf("H - print the help text\n");
    printf(":g <address> - goto <address>\n");
    printf(":q - quit\n");
    printf(":w - save to file\n");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("%sUsage: %s <filename>%s\n", LRED, argv[0], RESET);
        return 1;
    }

    if (!strcmp(argv[1], "--help")) { // if argv[1] == "--help"
        printHelp();
        return 0;
    }

    char* filename = argv[1];
    FILE* fptr = fopen(filename, "r+b");

    if (fptr == NULL) {
        printf("%sError while opening file '%s'.%s\n", LRED, filename, RESET);
        return 1;
    } 

    fseek(fptr, 0, SEEK_END);
    unsigned int fileSize = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    unsigned char* buffer = malloc(fileSize);

    if (buffer == NULL) {
        printf("%sError while allocating memory.%s\n", LRED, RESET);
        fclose(fptr);
        return 1;
    }

    fread(buffer, 1, fileSize, fptr);

    unsigned int selectedAddress = 0x0;
    unsigned int screenAddress = 0x0;

    while (1) {
        renderScreen(screenAddress, selectedAddress, 0, buffer, fileSize, filename);
        char input = getch();
        if (input == ':') {
            printf(":");
            char command = getch();
            printf("%c", command);

            if (command == 'q') {
                printf("\nAre you sure you want to quit? (y/n)");
                char confirmation = getch();
                if (confirmation == 'y') break;
            } else if (command == 'g') {
                printf(" %s", LMAGENTA);
                long long address = inputInt();
                if (address < fileSize && address != -1) selectedAddress = address;
                else if (address >= fileSize) {
                    printf("\n%sOut of file bounds. Press Enter to continue.%s", LRED, RESET);
                    waitForEnter();
                } else {
                    printf("\n%sInvalid address. Press Enter to continue.%s", LRED, RESET);
                    waitForEnter();
                }
            } else if (command == 'w') {
                printf("\nAre you sure you want to save the file? (y/n)");
                char confirmation = getch();
                if (confirmation == 'y') {
                    fseek(fptr, 0, SEEK_SET);
                    size_t bytesWritten = fwrite(buffer, 1, fileSize, fptr);
                    if (bytesWritten == fileSize) {
                        printf("\nSuccessfully wrote to file. Press Enter to continue.");
                    } else {
                        printf("\n%sError while writing to file. Press Enter to continue.%s", LRED, RESET);
                    }
                    waitForEnter();
                }
            } else {
                printf("\n%sUnknown command. Press Enter to continue.%s", LRED, RESET);
                waitForEnter();
            }
        } 
        else switch (input) {
            case 'w':
                if (selectedAddress >= 16) selectedAddress -= 16;
                break;
            case 's':
                if (selectedAddress + 16 < fileSize) selectedAddress += 16;
                break;
            case 'a':
                if (selectedAddress % 16 != 0) selectedAddress--;
                break;
            case 'd':
                if (selectedAddress + 1 < fileSize && selectedAddress % 16 != 15) selectedAddress++;
                break;
            case 'e':
                renderScreen(screenAddress, selectedAddress, 1, buffer, fileSize, filename);
                int value = inputByte();
                if (value > 0) buffer[selectedAddress] = value;
                else {
                    printf("%sInvalid value. Press Enter to continue.%s", LRED, RESET);
                    waitForEnter();
                }
                break;
            case 'c':
                renderScreen(screenAddress, selectedAddress, 2, buffer, fileSize, filename);
                buffer[selectedAddress] = getch();
                break;
            case 'H':
                system("clear");
                printHelp();
                printf("\nPress any key to continue.");
                getch();
                break;
            case 'W':
                {
                    char input;
                    while (1) {
                        renderScreen(screenAddress, selectedAddress, 4, buffer, fileSize, filename);
                        input = getch();
                        if (input == 27) break; // if esc is pressed
                        buffer[selectedAddress] = input;
                        if (selectedAddress != fileSize - 1) selectedAddress++;
                    }
                }
                break;
            case 'E':
                {
                    int input;
                    while (1) {
                        renderScreen(screenAddress, selectedAddress, 3, buffer, fileSize, filename);
                        input = inputByte();
                        if (0 <= input && input <= 255) {
                            buffer[selectedAddress] = input;
                            if (selectedAddress != fileSize - 1) selectedAddress++;
                        } else if (input == -27) break; // if esc is pressed
                        else {
                            printf("%sInvalid value. Press Enter to continue.%s", LRED, RESET);
                            waitForEnter();
                            break;
                        }
                    }
                }
                break;
        }

        while (screenAddress + (LINES_PER_SCREEN - 1) * 16 - 1  < selectedAddress) screenAddress += 16;
        while (screenAddress > selectedAddress - 16) screenAddress -= 16;

    }

    fclose(fptr);
    return 0;
}