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
} CommandMapping;

// Function prototypes
void displayMenu();
void displayManual();
void executeCommand(char *command);
void loadMappings(CommandMapping *mappings, int *mappingCount);
char *getDOSToLinuxCommand(char *dosCommand, CommandMapping *mappings, int mappingCount);
void trimString(char *str);

int main() {
    int choice = 0;
    char command[MAX_COMMAND_LENGTH];
    
    // Display welcome message
    printf("\n===== DOS Command Shell Simulator =====\n\n");
    
    // Main program loop
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
                printf("\nEnter DOS command: ");
                fgets(command, MAX_COMMAND_LENGTH, stdin);
                command[strcspn(command, "\n")] = 0; // Remove newline
                executeCommand(command);
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
        system("clear || cls"); // Clear screen (works on both Linux and Windows)
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
}

void executeCommand(char *command) {
    CommandMapping mappings[100];
    int mappingCount = 0;
    char *linuxCommand;
    char outputBuffer[MAX_OUTPUT_LENGTH];
    FILE *fp;
    
    // Load mappings from file
    loadMappings(mappings, &mappingCount);
    
    // Get the Linux equivalent of the DOS command
    linuxCommand = getDOSToLinuxCommand(command, mappings, mappingCount);
    
    if (linuxCommand == NULL) {
        printf("Error: Command '%s' not found in the mapping file.\n", command);
        return;
    }
    
    printf("Executing: %s (DOS) -> %s (Linux)\n", command, linuxCommand);
    
    // Execute the command and capture output
    fp = popen(linuxCommand, "r");
    if (fp == NULL) {
        printf("Error executing command.\n");
        return;
    }
    
    // Read and display output
    printf("\n----- Command Output -----\n");
    while (fgets(outputBuffer, MAX_OUTPUT_LENGTH, fp) != NULL) {
        printf("%s", outputBuffer);
    }
    printf("-------------------------\n");
    
    pclose(fp);
}

void loadMappings(CommandMapping *mappings, int *mappingCount) {
    FILE *file = fopen(MAPPING_FILE, "r");
    char line[MAX_MAPPING_LENGTH];
    char *delimiter;
    
    *mappingCount = 0;
    
    if (file == NULL) {
        printf("Warning: Mapping file not found. Using default mappings.\n");
        
        // Default mappings
        strcpy(mappings[*mappingCount].dos_command, "dir");
        strcpy(mappings[*mappingCount].linux_command, "ls -la");
        (*mappingCount)++;
        
        strcpy(mappings[*mappingCount].dos_command, "copy");
        strcpy(mappings[*mappingCount].linux_command, "cp");
        (*mappingCount)++;
        
        strcpy(mappings[*mappingCount].dos_command, "del");
        strcpy(mappings[*mappingCount].linux_command, "rm");
        (*mappingCount)++;
        
        strcpy(mappings[*mappingCount].dos_command, "md");
        strcpy(mappings[*mappingCount].linux_command, "mkdir");
        (*mappingCount)++;
        
        strcpy(mappings[*mappingCount].dos_command, "cd");
        strcpy(mappings[*mappingCount].linux_command, "cd");
        (*mappingCount)++;
        
        strcpy(mappings[*mappingCount].dos_command, "cls");
        strcpy(mappings[*mappingCount].linux_command, "clear");
        (*mappingCount)++;
        
        strcpy(mappings[*mappingCount].dos_command, "type");
        strcpy(mappings[*mappingCount].linux_command, "cat");
        (*mappingCount)++;
        
        return;
    }
    
    while (fgets(line, MAX_MAPPING_LENGTH, file) != NULL && *mappingCount < 100) {
        // Skip empty lines and comments
        if (line[0] == '\n' || line[0] == '#') {
            continue;
        }
        
        // Find the delimiter
        delimiter = strstr(line, "=");
        if (delimiter == NULL) {
            continue;
        }
        
        // Split the line into DOS command and Linux command
        *delimiter = '\0';
        strcpy(mappings[*mappingCount].dos_command, line);
        strcpy(mappings[*mappingCount].linux_command, delimiter + 1);
        
        // Trim whitespace
        trimString(mappings[*mappingCount].dos_command);
        trimString(mappings[*mappingCount].linux_command);
        
        (*mappingCount)++;
    }
    
    fclose(file);
}

char *getDOSToLinuxCommand(char *dosCommand, CommandMapping *mappings, int mappingCount) {
    int i;
    char dosCmd[MAX_COMMAND_LENGTH];
    char *args = NULL;
    static char fullLinuxCommand[MAX_COMMAND_LENGTH * 2];
    
    // Extract the command (without arguments)
    strcpy(dosCmd, dosCommand);
    args = strchr(dosCmd, ' ');
    if (args != NULL) {
        *args = '\0';
        args = strchr(dosCommand, ' ');  // Point back to original string
    }
    
    // Find the command in the mappings
    for (i = 0; i < mappingCount; i++) {
        if (strcasecmp(dosCmd, mappings[i].dos_command) == 0) {
            // Found the command
            strcpy(fullLinuxCommand, mappings[i].linux_command);
            
            // Add the arguments if any
            if (args != NULL) {
                strcat(fullLinuxCommand, args);
            }
            
            return fullLinuxCommand;
        }
    }
    
    return NULL;
}

void trimString(char *str) {
    int i, j;
    
    // Trim leading whitespace
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
    
    // Trim trailing whitespace
    i = strlen(str) - 1;
    while (i >= 0 && isspace((unsigned char)str[i])) {
        str[i] = '\0';
        i--;
    }
}
