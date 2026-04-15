/**
 * slosh.c 
 * 
 * SLOsh - San Luis Obispo Shell
 * CSC 453 - Operating Systems
 * Description: This file contains the main implementation of the SLOsh shell. It includes
 * the main loop, command parsing, execution, and signal handling.
 * 
 */

#include "slosh.h"

/* Global variable for signal handling */
volatile sig_atomic_t child_running = 0;

/**
 * Signal handler for SIGINT (Ctrl+C)
 *
 * TODO: Handle Ctrl+C appropriately. Think about what behavior makes sense
 * when the user presses Ctrl+C - should the shell exit? should a child process
 * be interrupted?
 * Hint: The global variable tracks important state.
 */
void sigint_handler(int sig) {
    (void)sig;

    if (!child_running) {
        write(STDOUT_FILENO, "\n", 1);
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
        char word[len + 1];
        strncpy(word, input + start, len);
        word[len] = '\0';
        args[i++] = word;
    }

    // Null-terminate the args array
    args[i] = NULL;
    return i;
}

/**
 * Helper function for execute_command that gets commands and organizes into command struct 
 * for easier execution
 * 
 * @param args Array of command arguments (NULL-terminated)
 * @param cmnds Array of command structs
 * @return Number of commands parsed
 * 
*/
int get_commands(char **args, command_t *cmnds){
    // initialize first command start
    cmnds[num_cmnds].argv = &args[0];
    cmnds[num_cmnds].outfile = NULL;
    cmnds[num_cmnds].append = 0;

    int i = 0; 
    int num_cmnds = 1;
    while(args[i] != NULL){

        // Get pipe commands
        if (strcmp(args[i], "|") == 0) {

            // Null terminate to split command args
            args[i] = NULL;  
            num_cmnds++;

            // Initialize next command 
            cmnds[num_cmnds].argv = &args[i + 1];
            cmnds[num_cmnds].outfile = NULL;
            cmnds[num_cmnds].append = 0;
        }

        // Get redirect command
        else if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0){
            if (args[i + 1] == NULL) {
                fprintf(stderr, "missing output file\n");
                return -1;
            }

            // Set append flag if command is >>
            if(strcmp(args[i], ">>") == 0) cmnds[num_cmnds].append = 1;

            // Null terminate to split command args
            args[i] = NULL;  
            cmnds[num_cmnds].outfile = args[i + 1];

            // Redirection is always last command so error if further commands 
            if (args[i + 2] != NULL) {
                fprintf(stderr, "invalid redirection syntax\n");
                return -1;
            }
            break;
        }
        i++;
    }

    return num_cmnds;
}

/**
 * Execute the given command with its arguments
 *
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
    command_t cmnds[MAX_ARGS];
    int num_cmnds = get_commands(args, cmnds);
    pid_t pids[MAX_ARGS];
    int prev_pipe = -1; 

    if (num_cmnds > 0) {
        child_running = 1;
    }
    
    // loop to fork according to number of pipes
    for(int i = 0; i < num_cmnds; i++){
        int pfds[2];

        // Create pipe for all but last command
        if (i < num_cmnds - 1){
            if (pipe(pfds) < 0) {
                perror("pipe");
                return;
            }
        }

        // Check for fork error
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return;
        }

        // Child process
        if (pids[i] == 0){

            // Child signal handler
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, NULL);

            // Read from pipe if not first command
            if(prev_pipe != -1){
                if (dup2(prev_pipe, STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
                close(prev_pipe);
            }

            // Write to next pipe if not last command 
            if (i < num_cmnds - 1){
                if (dup2(pfds[1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
                close(pfds[0]); 
                close(pfds[1]); 
            }

            // Redirect if last command has output file
            else if(cmnds[i].outfile != NULL){
                int fd;
                if (cmnds[i].append) {
                    fd = open(cmnds[i].outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    fd = open(cmnds[i].outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                if (fd < 0) {
                    perror("open");
                    exit(1);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(1);
                }
                close(fd);
            }

            //Execute command after dealing with special ops
            execvp(cmnds[i].argv[0], cmnds[i].argv);
            perror("execvp");
            exit(1);
        }

        // Parent process
        else{
            if (prev_pipe != -1) close(prev_pipe);
            if (i < num_cmnds - 1) {
                close(pfds[1]); // Parent doesn't write to current pipe
                prev_pipe = pfds[0]; // Save read end for next command
            }
        }
    }

    // Close any remaining pipe in parent
    if (prev_pipe != -1) {
        close(prev_pipe);
    }

    // Wait for children processes to finish
    for (int i = 0; i < num_cmnds; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            continue;
        }

        // Capture status
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "terminated by signal %d\n", WTERMSIG(status));
        }
        else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "exit status %d\n", WEXITSTATUS(status));
        }
    }
    child_running = 0;

    return;
}

/**
 * Check for and handle built-in commands
 *
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
            return 1;
        }
        else{
            perror("error changing directory");
            return -1;
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

    // Signal handeling setup for SIGINT (Ctrl+C)
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
            if (feof(stdin)) {
                printf("\n");
                break; // EOF, exit the shell
            } else if (ferror(stdin)) {
                perror("Error reading input");
                clearerr(stdin); // Clear error and continue
                continue;
            }
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