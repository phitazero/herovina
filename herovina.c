// HExadecimal RedactOr VersIoN Alpha (HeRoVinA)
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

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

typedef struct {
	uint64_t fileSize;
	uint64_t selectedAddress;
	uint64_t screenAddress;
	char* buffer;
	char* filename;
	FILE* fptr;
} Context;

void renderLine(Context* ctx, unsigned int lineAddress, uint8_t editMode) {
	printf("[%s%08X%s]  ", MAGENTA, lineAddress, RESET);

	char* selectionColors[] = {LMAGENTA, RED, YELLOW, LRED, LYELLOW};
	char* selectionColor = selectionColors[editMode];

	for (int offset = 0; offset < 16; offset++) {
		if (lineAddress + offset < ctx->fileSize) {
			unsigned char symbol = ctx->buffer[lineAddress + offset];
			if (lineAddress + offset == ctx->selectedAddress) printf(selectionColor);
			printf("%02X%s ", symbol, RESET);
		} else {
			printf("   ");
		}
	}

	printf(" | ");

	for (int offset = 0; offset < 16; offset++) {
		unsigned char symbol = ctx->buffer[lineAddress + offset];
		if (lineAddress + offset < ctx->fileSize && symbol >= 32 && symbol <= 126) {
			if (lineAddress + offset == ctx->selectedAddress) printf(selectionColor);
			printf("%c%s", symbol, RESET);
		} else {
			printf("%s.%s", LBLACK, RESET);
		}
	}
	printf("\n");
}

void renderScreen (Context* ctx, uint8_t editMode) {
	system("clear");

	printf("            %s0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F%s\n", MAGENTA, RESET);

	unsigned int lineAddress;
	for (int line = 0; line < LINES_PER_SCREEN && 16 * line + ctx->screenAddress < ctx->fileSize; line++) {
		lineAddress = ctx->screenAddress + 16 * line;
		renderLine(ctx, lineAddress, editMode);
	}

	printf("\n[%s%08X%s]", LMAGENTA, ctx->selectedAddress, RESET);
	printf("  (%s%s%s)", MAGENTA, ctx->filename, RESET);
	printf("  %d bytes", ctx->fileSize);
	printf(" (%s00000000%s - %s%08X%s)", LMAGENTA, RESET, LMAGENTA, ctx->fileSize - 1, RESET);
	printf("\n");
}

void waitForEnter() {
	while (1) {
		if (getch() == 0xA) break; // if getch() == enter
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
		if (input == '\n') break; // enter
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
	printf(":r - reload file\n");
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

	Context ctx;

	ctx.filename = argv[1];
	ctx.fptr = fopen(ctx.filename, "r+b");

	if (ctx.fptr == NULL) {
		printf("%sError while opening file '%s'.%s\n", LRED, ctx.filename, RESET);
		return 1;
	}

	fseek(ctx.fptr, 0, SEEK_END);
	ctx.fileSize = ftell(ctx.fptr);
	fseek(ctx.fptr, 0, SEEK_SET);

	ctx.buffer = malloc(ctx.fileSize);

	if (ctx.buffer == NULL) {
		printf("%sError while allocating memory.%s\n", LRED, RESET);
		fclose(ctx.fptr);
		return 1;
	}

	fread(ctx.buffer, 1, ctx.fileSize, ctx.fptr);

	ctx.selectedAddress = 0x0;
	ctx.screenAddress = 0x0;

	while (1) {
		renderScreen(&ctx, 0);
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
				if (address < ctx.fileSize && address != -1) ctx.selectedAddress = address;
				else if (address >= ctx.fileSize) {
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
					fseek(ctx.fptr, 0, SEEK_SET);
					size_t bytesWritten = fwrite(ctx.buffer, 1, ctx.fileSize, ctx.fptr);
					if (bytesWritten == ctx.fileSize) {
						printf("\nSuccessfully wrote to file. Press Enter to continue.");
					} else {
						printf("\n%sError while writing to file. Press Enter to continue.%s", LRED, RESET);
					}
					waitForEnter();
				}
			} else if (command == 'r') {
				fseek(ctx.fptr, 0, SEEK_END);
				ctx.fileSize = ftell(ctx.fptr);
				fseek(ctx.fptr, 0, SEEK_SET);

				ctx.buffer = realloc(ctx.buffer, ctx.fileSize);

				if (ctx.buffer == NULL) {
					printf("%sError while allocating memory.%s\n", LRED, RESET);
					fclose(ctx.fptr);
					return 1;
				}

				fread(ctx.buffer, 1, ctx.fileSize, ctx.fptr);

				// idk why it's required, prints a r symbol otherwise, messing up the TUI
				fflush(stdout);

				if (ctx.selectedAddress > ctx.fileSize - 1)
					ctx.selectedAddress = ctx.fileSize - 1;

			} else {
				printf("\n%sUnknown command. Run with --help flag for help.\nPress Enter to continue.%s", LRED, RESET);
				waitForEnter();
			}
		} 
		else switch (input) {
			case 'w':
				if (ctx.selectedAddress >= 16) ctx.selectedAddress -= 16;
				break;
			case 's':
				if (ctx.selectedAddress + 16 < ctx.fileSize) ctx.selectedAddress += 16;
				break;
			case 'a':
				if (ctx.selectedAddress > 0) ctx.selectedAddress--;
				break;
			case 'd':
				if (ctx.selectedAddress + 1 < ctx.fileSize) ctx.selectedAddress++;
				break;
			case 'e':
				renderScreen(&ctx, 1);
				int value = inputByte();
				if (value >= 0) ctx.buffer[ctx.selectedAddress] = value;
				else {
					printf("%sInvalid value. Press Enter to continue.%s", LRED, RESET);
					waitForEnter();
				}
				break;
			case 'c':
				renderScreen(&ctx, 2);
				ctx.buffer[ctx.selectedAddress] = getch();
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
						renderScreen(&ctx, 4);
						input = getch();
						if (input == 27) break; // if esc is pressed
						ctx.buffer[ctx.selectedAddress] = input;
						if (ctx.selectedAddress != ctx.fileSize - 1) ctx.selectedAddress++;
					}
				}
				break;
			case 'E':
				{
					int input;
					while (1) {
						renderScreen(&ctx, 3);
						input = inputByte();
						if (0 <= input && input <= 255) {
							ctx.buffer[ctx.selectedAddress] = input;
							if (ctx.selectedAddress != ctx.fileSize - 1) ctx.selectedAddress++;
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

		while (ctx.screenAddress + (LINES_PER_SCREEN - 1) * 16 - 1  < ctx.selectedAddress) ctx.screenAddress += 16;
		while (ctx.screenAddress > ctx.selectedAddress - 16) ctx.screenAddress -= 16;

	}

	fclose(ctx.fptr);
	printf("\n");
	return 0;
}
