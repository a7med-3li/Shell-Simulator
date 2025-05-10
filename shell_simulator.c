#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_MAPPING_LENGTH 100
#define MAX_OUTPUT_LENGTH 4096
#define MAPPING_FILE "dos_linux_mapping.txt"

typedef struct {
    char dos_command[MAX_COMMAND_LENGTH];
    char linux_command[MAX_COMMAND_LENGTH];
    int min_args;  // Minimum number of arguments required
    int max_args;  // Maximum number of arguments allowed
} CommandMapping;

void displayMenu();
void displayManual();
void executeCommand(char *command, FILE *file);
void loadMappings(CommandMapping *mappings, int *mappingCount, FILE *file);
char *getDOSToLinuxCommand(char *dosCommand, CommandMapping *mappings, int mappingCount, int *status);
void trimString(char *str);
int countArgs(char *command);
void getCommandArgumentRequirements(const char *dos_command, int *min_args, int *max_args);

// Status codes for command validation
#define CMD_OK 0
#define CMD_NOT_FOUND 1
#define CMD_TOO_FEW_ARGS 2
#define CMD_TOO_MANY_ARGS 3

int main() {
    int choice = 0;
    char command[MAX_COMMAND_LENGTH];
    
    printf("\n===== DOS Command Shell Simulator =====\n\n");

    while (1) {
        displayMenu();
        printf("\nEnter choice: ");
        if (scanf("%d", &choice) != 1) {
            // Clear input buffer if input is not an integer
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        // Clear input buffer
        getchar();
        
        switch (choice) {
            case 1:
                FILE *file = fopen(MAPPING_FILE, "r");
                if (file == NULL) {
                    printf("Warning: Mapping file not found. Try again....\n");
                }
                else{
                    printf("\nEnter DOS command: ");
                    fgets(command, MAX_COMMAND_LENGTH, stdin);
                    command[strcspn(command, "\n")] = 0; // Remove newline
                    executeCommand(command, file);
                }
                break;
            case 2:
                displayManual();
                break;
            case 3:
                printf("\nExiting DOS Command Shell Simulator...\n");
                return 0;
            default:
                printf("\nInvalid choice. Please try again.\n");
        }
        
        printf("\nPress Enter to continue...");
        getchar();
        system("clear || cls");
    }
    
    return 0;
}

void displayMenu() {
    printf("===== DOS Command Shell Simulator =====\n");
    printf("1. Execute DOS command\n");
    printf("2. View manual\n");
    printf("3. Exit\n");
}

void displayManual() {
    printf("\n===== DOS Command Shell Simulator Manual =====\n\n");
    printf("This shell simulates DOS commands by mapping them to equivalent Linux commands.\n");
    printf("When you enter a DOS command like 'dir', the shell executes the Linux equivalent 'ls'.\n\n");
    printf("The mapping is done using the external file '%s'.\n", MAPPING_FILE);
    printf("Format of mapping file: <DOS command> = <Linux command>\n\n");
    printf("Example commands:\n");
    printf("  - dir      : Lists files and directories (maps to 'ls')\n");
    printf("  - copy     : Copies files (maps to 'cp')\n");
    printf("  - del      : Deletes files (maps to 'rm')\n");
    printf("  - md       : Creates a directory (maps to 'mkdir')\n");
    printf("  - cd       : Changes directory\n");
    printf("  - cls      : Clears the screen (maps to 'clear')\n");
    printf("\nNote: If a command is not found in the mapping file, an error message will be displayed.\n");
    printf("      Some commands require specific numbers of arguments. Error messages will be shown\n");
    printf("      if too few or too many arguments are provided.\n");
}

void executeCommand(char *command, FILE *file) {
    CommandMapping mappings[100];
    int mappingCount = 0;
    char *linuxCommand;
    char outputBuffer[MAX_OUTPUT_LENGTH];
    FILE *fp;
    int status = CMD_OK;
    
    loadMappings(mappings, &mappingCount, file);
    
    linuxCommand = getDOSToLinuxCommand(command, mappings, mappingCount, &status);
    
    switch (status) {
        case CMD_NOT_FOUND:
            printf("Error: Command not found in the mapping file.\n");
            return;
        case CMD_TOO_FEW_ARGS:
            printf("Error: Too few arguments for command '%s'.\n", command);
            return;
        case CMD_TOO_MANY_ARGS:
            printf("Error: Too many arguments for command '%s'.\n", command);
            return;
    }
    
    printf("Executing: %s (DOS) -> %s (Linux)\n", command, linuxCommand);

    fp = popen(linuxCommand, "r");
    if (fp == NULL) {
        printf("Error executing command.\n");
        return;
    }

    printf("\n----- Command Output -----\n");
    while (fgets(outputBuffer, MAX_OUTPUT_LENGTH, fp) != NULL) {
        printf("%s", outputBuffer);
    }
    printf("-------------------------\n");
    
    pclose(fp);
}

void loadMappings(CommandMapping *mappings, int *mappingCount, FILE *file) {
    char line[MAX_MAPPING_LENGTH];
    char *delimiter;
    
    *mappingCount = 0;
    rewind(file);  // Reset file pointer to the beginning
    
    while (fgets(line, MAX_MAPPING_LENGTH, file) != NULL && *mappingCount < 100) {
        delimiter = strstr(line, "=");
        if (delimiter == NULL) {
            continue;
        }
        
        // Split the line into DOS command and Linux command
        *delimiter = '\0';
        strcpy(mappings[*mappingCount].dos_command, line);
        strcpy(mappings[*mappingCount].linux_command, delimiter + 1);

        trimString(mappings[*mappingCount].dos_command);
        trimString(mappings[*mappingCount].linux_command);
        
        // Set default argument requirements
        getCommandArgumentRequirements(mappings[*mappingCount].dos_command, 
                                      &mappings[*mappingCount].min_args, 
                                      &mappings[*mappingCount].max_args);
        
        (*mappingCount)++;
    }
}

char *getDOSToLinuxCommand(char *dosCommand, CommandMapping *mappings, int mappingCount, int *status) {
    int i;
    char dosCmd[MAX_COMMAND_LENGTH];
    char *args = NULL;
    static char fullLinuxCommand[MAX_COMMAND_LENGTH * 2];
    int argCount = 0;
    
    // Extract the command without arguments
    strcpy(dosCmd, dosCommand);
    args = strchr(dosCmd, ' ');
    if (args != NULL) {
        *args = '\0';
        args = strchr(dosCommand, ' ');
        argCount = countArgs(args);
    }

    for (i = 0; i < mappingCount; i++) {
        if (strcasecmp(dosCmd, mappings[i].dos_command) == 0) {
            // Check if the command has the correct number of arguments
            if (argCount < mappings[i].min_args) {
                *status = CMD_TOO_FEW_ARGS;
                return NULL;
            }
            if (argCount > mappings[i].max_args && mappings[i].max_args != -1) {
                *status = CMD_TOO_MANY_ARGS;
                return NULL;
            }
            
            strcpy(fullLinuxCommand, mappings[i].linux_command);

            if (args != NULL) {
                strcat(fullLinuxCommand, args);
            }
            
            *status = CMD_OK;
            return fullLinuxCommand;
        }
    }
    
    *status = CMD_NOT_FOUND;
    return NULL;
}

void trimString(char *str) {
    int i, j;
    i = 0;
    while (isspace((unsigned char)str[i])) {
        i++;
    }
    
    if (i > 0) {
        for (j = 0; str[i + j] != '\0'; j++) {
            str[j] = str[i + j];
        }
        str[j] = '\0';
    }
    
    i = strlen(str) - 1;
    while (i >= 0 && isspace((unsigned char)str[i])) {
        str[i] = '\0';
        i--;
    }
}

// Count the number of arguments in a command string
int countArgs(char *command) {
    int count = 0;
    int inArg = 0;
    
    if (command == NULL) {
        return 0;
    }
    
    // Skip leading space
    while (*command == ' ') {
        command++;
    }
    
    // Count arguments (space-separated tokens)
    while (*command) {
        if (*command == ' ') {
            inArg = 0;
        } else if (!inArg) {
            inArg = 1;
            count++;
        }
        command++;
    }
    
    return count;
}

// Define argument requirements for each DOS command
void getCommandArgumentRequirements(const char *dos_command, int *min_args, int *max_args) {
    // Default: no arguments required
    *min_args = 0;
    *max_args = -1;  // -1 means unlimited arguments
    
    // Commands that require specific number of arguments
    if (strcasecmp(dos_command, "copy") == 0 || 
        strcasecmp(dos_command, "xcopy") == 0 || 
        strcasecmp(dos_command, "move") == 0 ||
        strcasecmp(dos_command, "rename") == 0) {
        *min_args = 2;  // These commands need at least 2 arguments (source and destination)
        *max_args = 2;  // And they accept exactly 2 arguments
    }
    else if (strcasecmp(dos_command, "type") == 0 ||
             strcasecmp(dos_command, "del") == 0 ||
             strcasecmp(dos_command, "erase") == 0 ||
             strcasecmp(dos_command, "rmdir") == 0 ||
             strcasecmp(dos_command, "rd") == 0 ||
             strcasecmp(dos_command, "md") == 0 ||
             strcasecmp(dos_command, "taskkill") == 0) {
        *min_args = 1;  // These commands need at least 1 argument
        *max_args = -1; // But can accept multiple arguments
    }
    else if (strcasecmp(dos_command, "find") == 0) {
        *min_args = 2;  // find needs at least a pattern and a file
    }
    else if (strcasecmp(dos_command, "ping") == 0) {
        *min_args = 1;  // ping needs at least a host
        *max_args = -1; // Can have more arguments for options
    }
    else if (strcasecmp(dos_command, "echo") == 0) {
        *min_args = 1;  // echo needs something to echo
    }
    else if (strcasecmp(dos_command, "dir") == 0 ||
             strcasecmp(dos_command, "cls") == 0 ||
             strcasecmp(dos_command, "ver") == 0 ||
             strcasecmp(dos_command, "date") == 0 ||
             strcasecmp(dos_command, "time") == 0 ||
             strcasecmp(dos_command, "ipconfig") == 0 ||
             strcasecmp(dos_command, "netstat") == 0 ||
             strcasecmp(dos_command, "tasklist") == 0) {
        // These commands don't require arguments but can accept optional ones
        *min_args = 0;
        *max_args = -1;
    }
}
