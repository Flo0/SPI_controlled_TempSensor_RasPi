#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include "wiringPiSim.h"
#include "wiringPiSPISim.h"

#define ANSI_COLOR_GREEN "\x1b[92m"
#define ANSI_COLOR_CYAN "\x1b[96m"
#define ANSI_COLOR_WHITE "\x1b[97m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define OUTPUT_PATH "./out.txt"
#define WRITE_ACTION "w+"

#define SPEED 1000000
#define CHANNEL 0
#define CMD_BUFFER_LEN 50

typedef char bool;
enum {
    false, true
};


float targetTemp;
int interval;

bool loop = true;
bool consoleOutputEnabled = false;

// SPI data
unsigned char data[3] = {0, 0, 0};

// Command input buffer
char *commandBuffer;
char *callBuffer;
char *argumentBuffer;

// Commands (could be maybe const)
char *timeCMD;
char *tempCMD;
char *inputCMD;
char *stopCMD;

FILE *outputStream;

// Thread for command input
pthread_t inputProcessingThread;

bool setup();

void startLoop();

void *keyCheck(void *threadID);

void checkSensor();

void work(struct timeval *lastCheck, struct timeval *current, u_int64_t maxDeltaMS);

bool processInput();

void clearCommandBuffer();

int main() {

    if (!setup()) {
        return -1;
    }

    pthread_create(&inputProcessingThread, NULL, keyCheck, NULL);

    delay(1000);

    startLoop();

    pthread_join(inputProcessingThread, NULL);

    return 0;
}

void *keyCheck(void *threadID) {

    // Print command infos
    fprintf(stderr, ANSI_COLOR_CYAN"\n---------------- Commands: ----------------\n"ANSI_COLOR_RESET);
    fprintf(stderr,
            ANSI_COLOR_WHITE"time <seconds> "ANSI_COLOR_RESET" -> "ANSI_COLOR_GREEN" Redefines check interval. \n"ANSI_COLOR_RESET);
    fprintf(stderr,
            ANSI_COLOR_WHITE"temp <target>  "ANSI_COLOR_RESET" ->  "ANSI_COLOR_GREEN"Redefines target temp. \n"ANSI_COLOR_RESET);
    fprintf(stderr,
            ANSI_COLOR_WHITE"tout           "ANSI_COLOR_RESET" ->  "ANSI_COLOR_GREEN"Toggles console output on/off. \n"ANSI_COLOR_RESET);
    fprintf(stderr,
            ANSI_COLOR_WHITE"stop           "ANSI_COLOR_RESET" ->  "ANSI_COLOR_GREEN"Stops the program. \n"ANSI_COLOR_RESET);
    fprintf(stderr, ANSI_COLOR_CYAN"Type the command and press ENTER\n\n"ANSI_COLOR_RESET);

    // Command input loop
    while (processInput()) {
        // Clear command buffer
        clearCommandBuffer();
        // Read command from stdin stream
        fgets(commandBuffer, CMD_BUFFER_LEN, stdin);
        // Parse command for call and optional argument
        sscanf(commandBuffer, "%s %s", callBuffer, argumentBuffer);
    }

    loop = false;
    return NULL;
}

bool processInput() {
    // Get argument type in callBuffer
    memcpy(callBuffer, commandBuffer, 4);

    // Check if command is stop
    if (strcmp(callBuffer, stopCMD) == 0) {
        return false;
    }

    // Check if command is toggle input
    if (strcmp(callBuffer, inputCMD) == 0) {
        consoleOutputEnabled = !consoleOutputEnabled;
        if (consoleOutputEnabled) {
            fprintf(stderr, ANSI_COLOR_GREEN"Output is now enabled.\n"ANSI_COLOR_RESET);
        } else {
            fprintf(stderr, ANSI_COLOR_GREEN"Output is now disabled.\n"ANSI_COLOR_RESET);
        }
    }

    // Check if command is time
    if (strcmp(callBuffer, timeCMD) == 0) {
        long newTime = strtol(argumentBuffer, NULL, 10);
        interval = (int) newTime;
        fprintf(stderr, ANSI_COLOR_GREEN"Changed interval to %i seconds.\n"ANSI_COLOR_RESET, interval);
    }

    // Check if command is temp
    if (strcmp(callBuffer, tempCMD) == 0) {
        targetTemp = strtof(argumentBuffer, NULL);
        fprintf(stderr, ANSI_COLOR_GREEN"Changed target temp to %.2f °C.\n"ANSI_COLOR_RESET, targetTemp);
    }

    // Only return false if program should stop
    return true;
}

// Clears command/call and argument buffer
void clearCommandBuffer() {
    for (int i = 0; i < sizeof(commandBuffer); i++) {
        commandBuffer[i] = 0;
    }
    for (int i = 0; i < sizeof(callBuffer); i++) {
        callBuffer[i] = 0;
    }
    for (int i = 0; i < sizeof(argumentBuffer); i++) {
        argumentBuffer[i] = 0;
    }
}

bool setup() {
    // Buffer for full command
    commandBuffer = malloc(CMD_BUFFER_LEN);
    // Buffer for command type
    callBuffer = malloc(10);
    // Buffer for first argument
    argumentBuffer = malloc(CMD_BUFFER_LEN - sizeof(callBuffer));

    timeCMD = malloc(10);
    tempCMD = malloc(10);
    inputCMD = malloc(10);
    stopCMD = malloc(10);

    // Defining commands
    timeCMD = "time";
    tempCMD = "temp";
    inputCMD = "tout";
    stopCMD = "stop";

    if (wiringPiSPISetup(CHANNEL, SPEED) == -1) {
        printf("SPI setup error.\n");
        return false;
    }

    fprintf(stderr, "Sollwert eingeben: ");
    scanf("%f", &targetTemp);

    fprintf(stderr, "Zeitintervall eingeben (Sek): ");
    scanf("%i", &interval);

    return true;
}

void startLoop() {
    // Print start info
    fprintf(stderr, ANSI_COLOR_CYAN"Loop started\n\n"ANSI_COLOR_RESET);

    fprintf(stderr, "Target temp: "ANSI_COLOR_GREEN" %.2f °C\n"ANSI_COLOR_RESET, targetTemp);
    fprintf(stderr, "Interval:    "ANSI_COLOR_GREEN" %i s\n"ANSI_COLOR_RESET, interval);
    fprintf(stderr, "Console out: "ANSI_COLOR_GREEN" %s\n"ANSI_COLOR_RESET, consoleOutputEnabled ? "Yes" : "No");

    outputStream = fopen(OUTPUT_PATH, WRITE_ACTION);

    // Time structs
    struct timeval last, current;

    while (loop) {
        work(&last, &current, (u_int64_t) (interval * 1E3L));
    }

    fclose(outputStream);
    fprintf(stderr, ANSI_COLOR_CYAN"Loop stopped.\n"ANSI_COLOR_RESET);
}

void work(struct timeval *lastCheck, struct timeval *current, u_int64_t maxDeltaMS) {
    gettimeofday(current, NULL);

    // Calc time since last check for non blocking ops
    u_int64_t deltaMS = (current->tv_sec - lastCheck->tv_sec) * 1E3L + (current->tv_usec - lastCheck->tv_usec) / 1E3L;
    if (deltaMS >= maxDeltaMS) {
        checkSensor();
        gettimeofday(lastCheck, NULL);
    }
}

void checkSensor() {
    data[0] = 0;
    data[1] = 0;
    data[2] = 0;

    wiringPiSPIDataRW(CHANNEL, data, sizeof(data));

    int adc = data[1] << 8 | data[2];

    float temp = adc * 1E3F / 4095.0F * 330.0F / 1E3F;

    bool heaterOn = temp < targetTemp;
    u_int64_t rpiMS = millis();

    if (consoleOutputEnabled) {
        fprintf(stderr, "[%llu;%.2f;%i]\n", rpiMS, temp, heaterOn);
    }
    fprintf(outputStream, "[%llu;%.2f;%i]\n", rpiMS, temp, heaterOn);
}


