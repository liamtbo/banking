
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE


int count_token (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	Check for NULL string
	*	#2.	iterate through string counting tokens
	*		Cases to watchout for
	*			a.	string start with delimeter
	*			b. 	string end with delimeter
	*			c.	account NULL for the last token
	*	#3. return the number of token (note not number of delimeter)
	*/
    if (buf == NULL) {
        return 0;
    }
    char *buf_cpy = strdup(buf);
    char *saveptr;                  // keeps track of last \0
    // strtok_r puts a new line char at each delim
    char *token = strtok_r(buf_cpy, delim, &saveptr); // token == "ls"
    int count = 0;
    while(token) {
        token = strtok_r(NULL, delim, &saveptr); // token == -a
        count += 1;
    }
    free(buf_cpy);
    return count;
}


command_line str_filler (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	create command_line variable to be filled and returned
	*	#2.	count the number of tokens with count_token function, set num_token. 
    *           one can use strtok_r to remove the \n at the end of the line.
	*	#3. malloc memory for token array inside command_line variable
	*			based on the number of tokens.
	*	#4.	use function strtok_r to find out the tokens 
    *   #5. malloc each index of the array with the length of tokens,
	*			fill command_list array with tokens, and fill last spot with NULL.
	*	#6. return the variable.
	*/
    command_line com_line;


    // buf = strtok_r(buf, "\n", &tmp);
    int k = 0;
    while (buf[k] != '\0') {
        if (buf[k] == '\n') {
            buf[k] = '\0';
        }
        k++;
    }

    // parsing command
    // int num_token = count_token(buf, delim);
    com_line.num_token = count_token(buf, delim);
    com_line.command_list = (char**)malloc(sizeof(char*) * (com_line.num_token + 1)); // +1 is for NULL
    com_line.command_list[com_line.num_token] = NULL;

    if (com_line.command_list == NULL) {
        printf("mem allocation failed");
        exit(1);
    }
    char *saveptr;  // keeps track of last \0
    char *token = strtok_r(buf, delim, &saveptr); // token == "ls"
    int i = 0;
    while(token) {
        com_line.command_list[i] = (char *)malloc(sizeof(char) * (strlen(token) + 1)); // +1 for end of line char
        if (com_line.command_list[i] == NULL) {
            printf("mem allocation failed");
            exit(1);
        }
        strcpy(com_line.command_list[i], token);
        com_line.command_list[i][strlen(token)] = '\0';

        i += 1; // next delimeter
        token = strtok_r(NULL, delim, &saveptr); // token == -a
    }
    return com_line;
}

void free_command_line(command_line* command)
{
	//TODO：
	/*
	*	#1.	free the array base num_token
	*/
    for (int i = 0; i < command->num_token + 1; i++) { // +1 is for NULL
        free(command->command_list[i]);
    }
    free(command->command_list);
}
