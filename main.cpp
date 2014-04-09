/**
 * This application acts as a simple shell, allowing the user to execute
 * applications with specified parameters.
 *
 * TO COMPILE: gcc -o shelltest mainObj.cpp -lstdc++
 *
 * @author Ian Adamson
 * @version 11-05-2013
 */

#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define ARGLEN   100  // Maximum length of each argument
#define MAXARGS  20   // Maximum number of arguments
#define INPUTLEN 2048 // Maximum length of total user input 

/**
 * This object defines the shell and its methods.
 */
class Shell {
	protected:
		char *fullPath; // Untokenized PATH environment variable.
        
		int print_cwd();
		int run(const char*, char*[]);
		char* c_concat(const char*, const char*);
		char* make_string(char*);
	public:
		Shell();
		int execute(char*);
		int execute(char*, char*[]);
};

/**
 * Constructor.
 */
Shell::Shell() {
	fullPath = getenv("PATH"); 
}

/**
 * Attempts to execute a command sent by the user.
 *
 * @return An integer indicating success (1) or failure (0).
 */
int Shell::execute(char* input) {
	// Format the input
	input = make_string(input);

	char* args[MAXARGS+1];
	char* command;
	char* argbuffer;
	int   argcount = 0;

	// Grab the first item (the command)
	command = strtok(input, " ");
	if(command == NULL) return 0;
	
	// Tokenize the arguments.
	// Note that execv wants the first argument to be the command again.
	argbuffer = command;
	while (argbuffer != NULL)
	{
		args[argcount++] = argbuffer;
		argbuffer = strtok(NULL, " ");
	}
	args[argcount] = NULL;
	
	// Pass the command/arguments to the real execute method
	return execute(command, args);
}

/**
 * Attempts to execute a command sent by the user.
 *
 * @return An integer indicating success (1) or failure (0).
 */
int Shell::execute(char* command, char* args[]) {
	// Handle special cases first.
	if(strcmp(command,"") == 0 || command[0]=='#') return 1;
	if(strcmp(command,"exit") == 0 || strcmp(command,"quit") == 0) exit(0);
	if(strcmp(command,"pwd") == 0 || strcmp(command,"cwd") == 0) {
		print_cwd();
		return 1;
	}
	
	// No special cases; pass the command and arguments to the run method.
	if(run(command, args)) return 1;
	   
	// If we get this far, nothing worked and we probably failed to fork.
	return 0;
}

/**
 * Protected method called by the execute method when the user requests
 *	 the current working directory.
 *
 * @return An integer indicating success (1) or failure (0).
 */
int Shell::print_cwd() {
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s\n", cwd);
		return 1; // Success!
	}
	printf("ERROR! Unable to fetch present working directory!\n");
	return 0; // Error!
}

/**
 * Protected method called by the execute method that attempts to
 *	 actually run a specific process using fork and execv.
 *
 * @return An integer indicating success (1) or failure (0).
 */
int Shell::run(const char* command, char* args[]) {
	int status;
	int fork_rv = fork();
	if(fork_rv == 0) { // Child process
		// Attempt to execute command in current and bin directories.
		execv(c_concat("./",command), args);
		execv(c_concat("/usr/bin/",command), args);
		execv(c_concat("/bin/",command), args);

		// Attempt to execute command in each PATH directory.
		char* tokenizedPath;
		tokenizedPath = strtok(fullPath, ":");
		while (tokenizedPath != NULL)
		{
			execv(c_concat(tokenizedPath,command), args);
			tokenizedPath = strtok (NULL, ":");
		}
		
		printf("ERROR! Unrecognized command!\n");
		exit(0);		
	} else if(fork_rv > 0) { // Parent process
		wait(&status);
		return 1; // success
	}
	return 0; // failed to fork
}

/**
 * Concatenates two c-style strings and returns the result.
 *
 * @return A pointer to a null terminated c-style string.
 */
char* Shell::c_concat(const char* a, const char* b) {
	const size_t aLen	= strlen(a);
	const size_t bLen	= strlen(b);
	const size_t newSize = aLen + bLen + 1; // +1 for the null terminator
	
	// Allocate memory for the new string.
	char* concatStr = (char*) malloc(newSize);
	if(concatStr==NULL) { // Die with error if out of memory
		fprintf(stderr,"no memory\n");
		exit(1);
	}
	
	strcpy(concatStr, a); // Includes null terminator
	strcat(concatStr, b); // Copies over previous terminator
	
	// Return the pointer to the new string.
	return concatStr;
}

/**
 * Converts user input into a proper c-style string
 *
 * @return A pointer to a null terminated c-style string.
 */
char* Shell::make_string(char *buf) {
	char *cp;
	
	buf[strlen(buf)-1] = '\0';          // Replace newline with null terminator
	cp = (char*)malloc(strlen(buf)+1);  // Try to get some memory
	if(cp==NULL) {                      // Die with error if out of memory
		fprintf(stderr,"no memory\n");
		exit(1);
	}
	strcpy(cp,buf);                     // Copy buffer into return var
	return cp;                          // ... and then return it
}

/******************************************************************************
 * The main function creates a Shell object and then passes user input to it.
 ******************************************************************************/
int main () {
	// Initialize the Shell object and a char buffer to hold user input.
	Shell Shell;
	char command[INPUTLEN];
	
	// Print a shell prompt, get a user command, pass it to the Shell, repeat.
	while(1) {
		printf("[ian-shell-%d]$ ", getpid());
		if(fgets(command, INPUTLEN, stdin) && *command != '\n') {
			if(!Shell.execute(command)) {
				printf("ERROR! Unable to execute command!\n");
			}
		}
	}
	
	// Shouldn't get here; program should end when user sends an "exit" command.
	return 0;
}
