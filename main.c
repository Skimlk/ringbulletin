/*
	Ring Bulletin:
		- Monitors selected RSS feeds for intent to participate in bulletin board
		- Creates/edits HTML page based on participation
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fetch.h"
//#include "loadjson.h"
#include <cjson/cJSON.h>

#define PROGRAM_TITLE "ringbulletin"
#define CONFIG_PATH "./config.json"

void printUsage() {
	printf("\nUsage: %s [OPTION]...\n\n", PROGRAM_TITLE);
}

void printHelpText() {
	printUsage();
	printf("Options:\n");
	printf("  -h\tPrint this help text and exit.\n");
	printf("  -r\tReload all bulletin board files.\n");
}

void printError(char *msg) {
	fprintf(stderr, "%s: error: %s\n", PROGRAM_TITLE, msg);
}

int main(int argc, char **argv) {
	int c = 0;
	int reloadFlag = 0;
	while((c = getopt(argc, argv, "rh")) != -1) {
		switch(c) {
			case 'r':
				reloadFlag = 1;
				break;
			case 'h':
				printHelpText();
				return 0;
			defualt:
				abort();
		}
	}
	
	return 0;
}
