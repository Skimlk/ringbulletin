#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prompt.h"
#include "stringutils.h"

char *getInput() {
	char *input = NULL;
	size_t size = 0;
	if(getline(&input, &size, stdin) == -1) {
		free(input);

		if(feof(stdin))
			clearerr(stdin);
		
		return NULL;
	}

	input[strcspn(input, "\n")] = '\0';
	return input;
}

char *stringInputPrompt(char *message, char *defaultInput) {
	char *input = NULL;
	char *ret = NULL;
	bool inputIsEmpty;

	do {
		printf("%s", message);
		if(defaultInput != NULL)
			ret = strdup(defaultInput);

		if(defaultInput != NULL && defaultInput[0] != '\0')
			printf(" (%s): ", defaultInput);
		else
			printf(": ");

		fflush(stdout);
		input = getInput();
		inputIsEmpty = input == NULL || input[0] == '\0';
		if(!inputIsEmpty) {
			free(ret);
			ret = strdup(input);
		}
		free(input);
	} while(defaultInput == NULL && inputIsEmpty);

	return ret;
}

bool boolInputPrompt(char *message, bool defaultInput) {
	printf("%s ", message);
	if(defaultInput)
		printf("(Y/n): ");
	else
		printf("(N/y): ");

	fflush(stdout);
	char *input = getInput();
	if(input == NULL)
		return defaultInput;

	if(input[0] != '\0') {
		bool answer = tolower((unsigned char)input[0]) == 'y';
		free(input);
		return answer;
	}

	free(input);
	return defaultInput;
}

int intInputPrompt(char *message, int *defaultInput) {
	char *input = NULL;
	bool inputIsEmpty;
	bool stringIsInt;
	int ret;

	do {
		printf("%s", message);
		if(defaultInput != NULL) {
			ret = *defaultInput;
			printf(" (%d): ", *defaultInput);
		}
		else
			printf(": ");

		fflush(stdout);
		input = getInput();
		stringIsInt = stringToInt(input, &ret) == 0;
		inputIsEmpty = input == NULL || input[0] == '\0';
		free(input);
	} while(!(defaultInput != NULL && inputIsEmpty) && !stringIsInt);

	return ret;
}
