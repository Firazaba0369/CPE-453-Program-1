/**
 * slosh.c 
 * 
 * SLOsh - San Luis Obispo Shell
 * CSC 453 - Operating Systems
 * Description: This file contains the main implementation of the SLOsh shell. It includes
 * the main loop, command parsing, execution, and signal handling.
 * 
 * TODO: Complete the implementation according to the comments
 */

#include "slosh.h"

/* Global variable for signal handling */
volatile sig_atomic_t child_running = 0;

/* Forward declarations */
void display_prompt(void);

/**
 * Signal handler for SIGINT (Ctrl+C)
 *
 * TODO: Handle Ctrl+C appropriately. Think about what behavior makes sense
 * when the user presses Ctrl+C - should the shell exit? should a child process
 * be interrupted?
 * Hint: The global variable tracks important state.
 */
void sigint_handler(int sig) {
    /* TODO: Your implementation here */
    if (sig == SIGINT){
        //Kill child process if running, otherwise ignore
        if (child_running) {

        }
    }
}

/**
 * Display the command prompt with current directory
 */
void display_prompt(void) {
    char cwd[PATH_MAX];
    char prompt_buf[PATH_MAX + 3];
    int len;

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        len = snprintf(prompt_buf, sizeof(prompt_buf), "%s> ", cwd);
    } else {
        len = snprintf(prompt_buf, sizeof(prompt_buf), "SLOsh> ");
    }

    if (len > 0 && len < (int)sizeof(prompt_buf)) {
        write(STDOUT_FILENO, prompt_buf, len);
    }
}

/**
 * Parse the input line into command arguments
 *
 * Extract tokens from the input string. What should you do with special
 * characters like pipes and redirections? How will the rest of the code know
 * what to execute?
 * Hint: You'll need to handle more than just splitting on spaces.
 *
 * @param input The input string to parse
 * @param args Array to store parsed arguments
 * @return Number of arguments parsed
 */
int parse_input(char *input, char **args) {
    int i = 0;
    int pos = 0;

    while (input[pos] != '\0' && i < MAX_ARGS - 1) {
        // Skip whitespace
        while (isspace((unsigned char)input[pos])) {
            pos++;
        }
        
        // Check for end of input after skipping whitespace
        if (input[pos] == '\0') {
            break;
        }

        // Handle |
        if (input[pos] == '|') {
            args[i++] = strdup("|");
            pos++;
            continue;
        }

        // Handle > and >>
        if (input[pos] == '>') {
            if (input[pos + 1] == '>') {
                args[i++] = strdup(">>");
                pos += 2;
            } else {
                args[i++] = strdup(">");
                pos++;
            }
            continue;
        }

        // Handle normal word
        int start = pos;
        while (input[pos] != '\0' &&
               !isspace((unsigned char)input[pos]) &&
               input[pos] != '|' &&
               input[pos] != '>') {
            pos++;
        }

        // Extract the word
        int len = pos - start;
        char *word = malloc(len + 1);
        strncpy(word, input + start, len);
        word[len] = '\0';
        args[i++] = word;
    }

    // Null-terminate the args array
    args[i] = NULL;
    return i;
}

/**
 * Execute the given command with its arguments
 *
 * TODO: Run the command. Your implementation should handle:
 * - Basic command execution
 * - Pipes (|)
 * - Output redirection (> and >>)
 *
 * What system calls will you need? How do you connect processes together?
 * How do you redirect file descriptors?
 *
 * @param args Array of command arguments (NULL-terminated)
 */
void execute_command(char **args) {
    /* TODO: Your implementation here */
}

/**
 * Check for and handle built-in commands
 *
 * TODO: Implement support for built-in commands:
 * - exit: Exit the shell
 * - cd: Change directory
 *
 * @param args Array of command arguments (NULL-terminated)
 * @return 0 to exit shell, 1 to continue, -1 if not a built-in command
 */
int handle_builtin(char **args) {
    // Check for cd command
    if (strcmp(args[0], "cd") == 0){
        if (chdir(args[1]) == 0){
            // printf("Current working directory updated");
            return 1
        }
        else{
            perror("error changing directory");
            return -1
        }
        
    }

    // Check for exit command
    if (strcmp(args[0], "exit") == 0){
        return 0;
    }

    return -1;  /* Not a builtin command */
}

int main(void) {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    int status = 1;
    int builtin_result;

    /* TODO: Set up signal handling. Which signals matter to a shell? */
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; 
    sigaction(SIGINT, &sa, NULL);

    while (status) {
        display_prompt();

        /* Read input and handle signal interruption */
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            /* TODO: Handle the case when fgets returns NULL. When does this happen? */
            break;
        }

        /* Parse input */
        parse_input(input, args);

        /* Handle empty command */
        if (args[0] == NULL) {
            printf("No command entered. Please try again.\n");
            continue;
        }

        /* Check for built-in commands */
        builtin_result = handle_builtin(args);
        if (builtin_result >= 0) {
            status = builtin_result;
            continue;
        }

        /* Execute external command */
        execute_command(args);
    }

    printf("SLOsh exiting...\n");
    return EXIT_SUCCESS;
}
