#include <iostream>
using namespace std;
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#define B_SIZE 1024
#define DELIMITER " "
#define READING 0x000
#define WRITING 0x001

/*

    Takes input received from user and checks it for errors. If the input is valid and does not contain errors, 0 is returned. If there are errors, then 1 is returned. If the user enters the exit command "exit", then 2 is returned.

*/
int parser(char *argv[50])
{
    bool pipe_used = false;
    bool background_used = false;
    bool i_o_used = false;
    bool special_pipe_used = false;

    // Immediately flag a command with length less than two
    if (strlen(argv[0]) == 0 || strlen(argv[0]) == 1)
    {
        cout << "Error: please enter a valid command ! \n" << endl;
        return 1;
    }

    // Exit command to close shell
    if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "Exit") == 0 || strcmp(argv[0], "EXIT") == 0)
    {
        return 2;
    } 
        
    int i = 1;
    while(argv[i] != NULL)
    {
        // Flag any commands that have |, $, or <,>
        if (strcmp(argv[i], "$") == 0 && (strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], ">") == 0 || strcmp(argv[i - 1], "$") == 0))
        {
            cout << "Error: the symbol " << argv[i] << " is in an invalid position ! \n" << endl;
            return 1;
        }
        else if (strcmp(argv[i], "|") == 0 && (strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], ">") == 0 || strcmp(argv[i - 1], "$") == 0))
        {
            cout << "Error: the symbol " << argv[i] << " is in an invalid position ! \n" << endl;
            return 1;
        }
        else if (strcmp(argv[i], "<") == 0 && (strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], ">") == 0 || strcmp(argv[i - 1], "$") == 0))
        {
            cout << "Error: the symbol " << argv[i] << " is in an invalid position ! \n" << endl;
            return 1;
        }
        else if (strcmp(argv[i], ">") == 0 && (strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], ">") == 0 || strcmp(argv[i - 1], "$") == 0))
        {
            cout << "Error: the symbol " << argv[i] << " is in an invalid position ! \n" << endl;
            return 1;
        }
        else if (strcmp(argv[i], "&") == 0 && (strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], ">") == 0 || strcmp(argv[i - 1], "$") == 0))
        {
            cout << "Error: the symbol " << argv[i - 1] << " is in an invalid position ! \n" << endl;
            return 1;
        }
        
        // Flag any commands that attempt to use multiple pipes in a single command
        if(strcmp(argv[i], "|") == 0 && (pipe_used == true))
        {
            cout << "Error: you cannot use multiple pipes in a single command ! \n" << endl;
            return 1;
        }
        // Flag any command that attempts to use the & symbol more than once
        else if (strcmp(argv[i], "&") == 0 && (background_used == true))
        {
            cout << "Error: you cannot use multiple & in a single command ! \n" << endl;
            return 1;
        }
        
        // Flag any command that attempts to use "double inputs", meaning a pipe then a command then an input redirection because the two things (pipe and <) contradict each other when used in that order
        if (strcmp(argv[i - 1], "|") == 0 && strcmp(argv[i + 1], "<") == 0)
        {
            cout << "Error: you've provided dual inputs for command " << argv[i] << ". You cannot precede a command with | and succeed it with < ! \n" << endl;
            return 1;
        }

        // Make note if a pipe or the background symbol has been used already
        if (strcmp(argv[i], "|") == 0)
            pipe_used = true;
        if (strcmp(argv[i], "&") == 0)
            background_used = true;
        if (strcmp(argv[i], "<") == 0 || strcmp(argv[i], ">") == 0)
            i_o_used = true;
        if (strcmp(argv[i], "$") == 0) 
            special_pipe_used = true;
            i++;
    }

    // Flag if a command ends with |, $, or <,>
    if (strcmp(argv[i - 1], "$") == 0 || strcmp(argv[i - 1], "|") == 0 || strcmp(argv[i - 1], "<") == 0 || strcmp(argv[i - 1], ">") == 0)
    {
        cout << "Error: the command " << argv[i - 1] << " must be succeeded by something ! \n" << endl;
        return 1;
    }

    // Flag if a command attempts to use the & symbol in the middle of the line as opposed to the end
    if(background_used == true)
    {
        for( int x = 0; x < i - 1; x++)
        {
            if(strcmp(argv[x],"&") == 0)
            {
                cout << "Error: the command & cannot be used within a command ! Only at the end ! \n" << endl;
                return 1;
            }
        }
    }

    // Flag if a command attempts to misuse the input/output redirection commands. Specifically using them in the wrong order, so any format not command -> file.
    for( int x = 0; x < i; x++)
    {
        if (strcmp(argv[x], "<") == 0 || strcmp(argv[x], ">") == 0)
        {
            for (int y = 0; y < strlen(argv[x - 1]); y++)
            {
                if(argv[x - 1][y]=='.')
                {
                    cout << "Error: input/output redirection used in the wrong order! Use in order command -> file! \n" << endl;
                    return 1;
                }
            }
        }
    }

    // Flag if a command attempts to use the $ operator incorrectly by having files instead of commands. Also flag if the $ operator is used in any other format than the three possible ones (cmd1 $ cmd2 cmd3, cmd1 cmd2 $ cmd3, cmd1 cmd2 $ cmd3 cmd4). Lastly if a command attempts to use $ in concert with other symbols.
    if(special_pipe_used == true)
    {
        int special_pipe_index;

        if(pipe_used == true || i_o_used == true)
        {
            cout << "Error: you cannot use special pipe $ in concert with other commands! \n" <<  endl;
            return 1;
        }

        int x = 0;
        while (argv[x] != NULL)
        {
            for (int y = 0; y < strlen(argv[x]); y++)
            {
                if (argv[x][y] == '.')
                {
                    cout << "Error: you cannot use files with the $ command! \n"
                         << endl;
                    return 1;
                }
            }

            if (strcmp(argv[x],"$") == 0) special_pipe_index = x;
            x++;
        }

        if (special_pipe_index == 1)
        {
            if (background_used == true && x != 5)
            {
                cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                     << endl;
                return 1;
            }
            else if (background_used == false && x != 4)
            {
                cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                     << endl;
                return 1;
            }
            else 
            {
            }
        }
        else if (special_pipe_index == 2)
        {
            if (background_used == true && (x != 5 && x != 6))
            {
                cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                     << endl;
                return 1;
            }
            else if (background_used == false && (x != 4 && x != 5))
            {
                cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                     << endl;
                return 1;
            }
            else
            {
            }
        }
        else 
        {
            cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                 << endl;
            return 1;
        }
        
    }
    
    return 0;

}

/*

    This function checks what operators the command uses of |, <, >,$. It then returns either 0, 1, 2, 3 or 4. 0 means it doesn't contain any, and 1, 2, 3, 4 coincide with |, <, >, $.

*/
int checkOperators(char *argv[50])
{
    int i = 0;
    while(argv[i] != NULL)
    {
        if (strcmp(argv[i],"|") == 0)
        {
            return 1;
        }
        else if (strcmp(argv[i], "<") == 0)
        {
            return 2;
        }
        else if (strcmp(argv[i], ">") == 0)
        {
            return 3;
        }
        else if (strcmp(argv[i], "$") == 0)
        {
            return 4;
        }
        i++;
    }

    return 0;
}

/*

    This function finds the index of the operator in argv

*/
int findOperatorIndex(char *argv[50], int op_code)
{
    int i = 0;
    while(argv[i] != NULL)
    {
        if (op_code == 1)
        {
            if(strcmp(argv[i], "<") == 0)
            {
                return i;
            }
        }
        else if (op_code == 2)
        {
            if(strcmp(argv[i], ">") == 0)
            {
                return i;
            }
        }
        else if (op_code == 3)
        {
            if(strcmp(argv[i], "|") == 0)
            {
                return i;
            }
        }  
        else if (op_code == 4)
        {
            if (strcmp(argv[i], "$") == 0)
            {
                return i;
            }
        } 
        i++;
    }
    return -1;
}

/*

    This command separates strings before the operator index, from the strings after it. Then returns a new string made up of just the strings before the operator index.  

*/
void findNewArgs(char* argv[50], int operator_index, char* new_args[50])
{
    int i = 0;
    while(i < operator_index){
        new_args[i] = argv[i];
        i++;
    }

    new_args[i] = NULL;
}

/*

    Split the arguments either side of a | command

*/
void splitArgs(char *argv[50], int operator_index, char *left[25], char *right[25])
{
    int i = 0;
    while (i < operator_index)
    {
        left[i] = argv[i];
        i++;
    }
    left[i] = NULL;
    i+=1;

    int x = 0;
    while (argv[i] != NULL)
    {
        right[x] = argv[i];
        i++;
        x++;
    }
    right[x] = NULL;
}


/*

    This function splits up the commands for the $ operator into cmd1, cmd2, cmd3 and cmd4 if there is a cmd4

*/
int splitArgs2(char *argv[50], int operator_index, char *cmd1[25], char *cmd2[25], char *cmd3[25], char *cmd4[25])
{
    // Count the number of commands in argv
    int number_of_commands = 0;
    while (argv[number_of_commands] != NULL)
    {
        number_of_commands++;
    }

    // Decrement by one because we don't include $ as one of the commands
    number_of_commands--;

    if (operator_index == 1)
    {
        // Setting commands based on number of arguments in argv
        cmd1[0] = argv[0];
        cmd1[1] = NULL;

        cmd2[0] = argv[2];
        cmd2[1] = NULL;

        cmd3[0] = argv[3];
        cmd3[1] = NULL;

        cmd4[0] = NULL;
    }
    else
    {
        if (number_of_commands == 3)
        {
            // Setting commands based on number of arguments in argv
            cmd1[0] = argv[0];
            cmd1[1] = NULL;

            cmd2[0] = argv[1];
            cmd2[1] = NULL;

            cmd3[0] = argv[3];
            cmd3[1] = NULL;

            cmd4[0] = NULL;
        }
        else
        {
            // Setting commands based on number of arguments in argv
            cmd1[0] = argv[0];
            cmd1[1] = NULL;

            cmd2[0] = argv[1];
            cmd2[1] = NULL;

            cmd3[0] = argv[3];
            cmd3[1] = NULL;

            cmd4[0] = argv[4];
            cmd4[1] = NULL;
        }
        
    }
   return number_of_commands;
}

    int main()
{
    while(1)
    {
        // I/O that solicits and collects commands from user
        cout << "mysh% ";

        // Splitting of characters adapted from code written in tutorial
        char line[B_SIZE];
        fgets(line, B_SIZE, stdin);
        line[strlen(line) - 1] = '\0';

        char *splits = strtok(line, " ");
        char *argv[50];

        int i = 0;
        while (splits != NULL)
        {
            argv[i] = splits;
            splits = strtok(NULL, DELIMITER);
            i++;
        }
        argv[i] = NULL;

        // Parse input
        int parse_response = parser(argv);

        // If command was valid
        if(parse_response == 0)
        {
            // If command has & we need to make note and run it in background
            bool run_background = false;

            // Remove the & command from args if it exists and set bool true
            if (strcmp(argv[i - 1], "&") == 0)
            {
                run_background = true;
                argv[i - 1] = NULL;
                i--;
            }

            int operator_response = checkOperators(argv);

            // Single commands with no operators
            if (operator_response == 0)
            {
                // Fork and execute command
                int status;
                int pid = fork();

                if (pid == 0)
                {
                    cout << endl;
                    execvp(argv[0], argv);
                }
                // Wait for child if & was used
                else if (pid > 0 && run_background == false)
                {
                    wait(&status);
                }
            }
            // Any | commands
            else if (operator_response == 1)
            {
                // Find where the | is
                int operator_index = findOperatorIndex(argv, 3);
                char *left[25];
                char *right[25];
                // Separate args either side of |
                splitArgs(argv, operator_index, left, right);

                // Create a pipe
                int fd[2];
                pipe(fd);

                // Save the fileno of the original stdin and stdout so we can restore it afterwards
                int original_in = dup(fileno(stdin));
                int original_out = dup(fileno(stdout));

                // Fork for right side execution 
                int status;
                int pid = fork();

                if(pid == 0)
                {
                    // Set standard input to read end of pipe
                    dup2(fd[0], 0);

                    // Flush the output buffer
                    fflush(stdout);

                    // Restore the stdout to the command shell from the write end of the pipe
                    dup2(original_out, 1);
                    close(original_out);

                    // Close both pipe ends
                    close(fd[0]);
                    close(fd[1]);

                    // Execute the command on the right side of the | 
                    execvp(right[0], right);
                }
                else
                {
                    // Fork for left side execution
                    int pid2 = fork();
                    int status2;

                    if (pid2 == 0)
                    {
                        // Set the write end of the pipe to be our standard output so result of execution of command gets written to pipe
                        dup2(fd[1], 1);

                        // Close both pipe ends
                        close(fd[0]);
                        close(fd[1]);

                        // Execute the command on the left side of the |
                        execvp(left[0], left);
                    }
                    else if (pid2 > 0)
                    {
                        // Close both pipe ends and wait for the child process to complete
                        close(fd[0]);
                        close(fd[1]);
                        waitpid(pid2, &status2, 0);
                    }

                    // Wait for child if & was used
                    if(run_background == false)
                    {
                        // Close both pipe ends
                        close(fd[0]);
                        close(fd[1]);

                        waitpid(pid, &status, 0);
                    }                    
                }

                // Flush the input buffer
                fflush(stdin);

                // Restore the stdin to the keyboard
                dup2(original_in, 0);
                close(original_in);
    
            }
            // Any < commands
            else if (operator_response == 2)
            {
                // Find where the < is
                int operator_index = findOperatorIndex(argv, 1);
                char *new_args[50];
                // Separate args left of < into new variable
                findNewArgs(argv, operator_index, new_args);

                // Save the fileno of the original stdin so we can restore it afterwards
                int original_in = dup(fileno(stdin));

                // Open the file given for use in i/o redirection
                int in = open(argv[i - 1], READING | O_CREAT, 0600);
                if (in == -1)
                {
                    cout << "Error: opening file \n" << endl;
                }

                // Make the stdin to the file given in commmand
                dup2(in, 0);
                close(in);

                // Fork and execute command
                int status;
                int pid = fork();

                if (pid == 0)
                {
                    cout << endl;
                    execvp(new_args[0], new_args);
                }
                // Wait for child if & was used
                else if (pid > 0 && run_background == false)
                {
                    wait(&status);
                }

                // Flush the input buffer
                fflush(stdin);

                // Restore the stdin to the keyboard
                dup2(original_in, 0);
                close(original_in);
            }
            // Any > commands
            else if (operator_response == 3)
            {
                // Find where the > is
                int operator_index = findOperatorIndex(argv, 2);
                char *new_args[50];
                // Separate args left of < into new variable
                findNewArgs(argv, operator_index, new_args);

                // Save the fileno of the original stdout so we can restore it afterwards
                int original_out = dup(fileno(stdout));

                // Open the file given for use in i/o redirection
                int out = open(argv[i - 1], WRITING | O_CREAT, 0600);
                if (out == -1)
                {
                    cout << "Error: opening file \n"
                         << endl;
                }

                // Make stdout the file given in command
                dup2(out, 1);

                // Fork and execute command
                int status;
                int pid = fork();

                if (pid == 0)
                {
                    cout << endl;
                    execvp(new_args[0], new_args);
                }
                // Wait for child if & was used
                else if (pid > 0 && run_background == false)
                {
                    wait(&status);
                }

                // Flush the output buffer and close out file
                fflush(stdout);
                close(out);

                // Restore the stdout to the command shell
                dup2(original_out, 1);
                close(original_out);
            }
            // Any $ commands
            else if (operator_response == 4)
            {
                int operator_index = findOperatorIndex(argv, 4);
                char *cmd1[25];
                char *cmd2[25];
                char *cmd3[25];
                char *cmd4[25];
                int number_of_commands = splitArgs2(argv,operator_index, cmd1,cmd2,cmd3,cmd4);

                // Create a pipe
                int fd[2];
                pipe(fd);

                // Save the fileno of the original stdin and stdout so we can restore it afterwards
                int original_in = dup(fileno(stdin));
                int original_out = dup(fileno(stdout));

                if (operator_index == 1)
                {
                    // Fork for cmd3
                    int status;
                    int pid = fork();

                    if (pid == 0)
                    {
                        // Close both pipe ends
                        close(fd[0]);
                        close(fd[1]);

                        // Execute cmd3
                        execvp(cmd3[0], cmd3);
                    }
                    else
                    {
                        // Fork for cmd2
                        int status2;
                        int pid2 = fork();

                        if(pid2 == 0)
                        {
                            // Set standard input to read end of pipe
                            dup2(fd[0], 0);

                            // Flush the output buffer
                            fflush(stdout);

                            // Restore the stdout to the command shell from the write end of the pipe
                            dup2(original_out, 1);
                            close(original_out);

                            // Close both pipe ends
                            close(fd[0]);
                            close(fd[1]);

                            // Execute cmd2
                            execvp(cmd2[0], cmd2);
                        }
                        else
                        {
                            // Fork for cmd1
                            int status3;
                            int pid3 = fork();

                            if(pid3 == 0)
                            {
                                // Set the write end of the pipe to be our standard output so result of execution of command gets written to pipe
                                dup2(fd[1], 1);

                                // Close both pipe ends
                                close(fd[0]);
                                close(fd[1]);

                                // Execute cmd1
                                execvp(cmd1[0], cmd1);
                            }
                            else 
                            {
                                // Close both pipe ends and wait for the child process to complete
                                close(fd[0]);
                                close(fd[1]);
                                waitpid(pid3, &status3, 0);
                            }

                            // Close both pipe ends and wait for the child process to complete
                            close(fd[0]);
                            close(fd[1]);
                            waitpid(pid2, &status2, 0);
                        }

                        // Wait for child if & was used
                        if (run_background == false)
                        {
                            // Close both pipe ends
                            close(fd[0]);
                            close(fd[1]);

                            waitpid(pid, &status, 0);
                        }
                    }

                    // Flush the input buffer
                    fflush(stdin);

                    // Restore the stdin to the keyboard
                    dup2(original_in, 0);
                    close(original_in);
                }
                else
                {
                    if (number_of_commands == 3)
                    {
                        // Fork for cmd3
                        int status;
                        int pid = fork();

                        if (pid == 0)
                        {
                            // Set standard input to read end of pipe
                            dup2(fd[0], 0);

                            // Flush the output buffer
                            fflush(stdout);

                            // Restore the stdout to the command shell from the write end of the pipe
                            dup2(original_out, 1);
                            close(original_out);
                            
                            // Close both pipe ends
                            close(fd[0]);
                            close(fd[1]);

                            // Execute cmd3
                            execvp(cmd3[0], cmd3);
                        }
                        else
                        {
                            // Fork for cmd2
                            int status2;
                            int pid2 = fork();

                            if (pid2 == 0)
                            {
                                // Close both pipe ends
                                close(fd[0]);
                                close(fd[1]);

                                // Execute cmd2
                                execvp(cmd2[0], cmd2);
                            }
                            else
                            {
                                // Fork for cmd1
                                int status3;
                                int pid3 = fork();

                                if (pid3 == 0)
                                {
                                    // Set the write end of the pipe to be our standard output so result of execution of command gets written to pipe
                                    dup2(fd[1], 1);

                                    // Close both pipe ends
                                    close(fd[0]);
                                    close(fd[1]);

                                    // Execute cmd1
                                    execvp(cmd1[0], cmd1);
                                }
                                else
                                {
                                    // Close both pipe ends and wait for the child process to complete
                                    close(fd[0]);
                                    close(fd[1]);
                                    waitpid(pid3, &status3, 0);
                                }

                                // Close both pipe ends and wait for the child process to complete
                                close(fd[0]);
                                close(fd[1]);
                                waitpid(pid2, &status2, 0);
                            }

                            // Wait for child if & was used
                            if (run_background == false)
                            {
                                // Close both pipe ends
                                close(fd[0]);
                                close(fd[1]);

                                waitpid(pid, &status, 0);
                            }
                        }

                        // Flush the input buffer
                        fflush(stdin);

                        // Restore the stdin to the keyboard
                        dup2(original_in, 0);
                        close(original_in);
                    }
                    else 
                    {
                        // Fork for cmd4
                        int status4;
                        int pid4 = fork();

                        if (pid4 == 0)
                        {
                            // Close both pipe ends
                            close(fd[0]);
                            close(fd[1]);

                            // Execute cmd4
                            execvp(cmd4[0], cmd4);
                        }
                        else
                        {
                            // Fork for cmd3
                            int status3;
                            int pid3 = fork();

                            if (pid3 == 0)
                            {
                                // Set standard input to read end of pipe
                                dup2(fd[0], 0);

                                // Flush the output buffer
                                fflush(stdout);

                                // Restore the stdout to the command shell from the write end of the pipe
                                dup2(original_out, 1);
                                close(original_out);

                                // Close both pipe ends
                                close(fd[0]);
                                close(fd[1]);

                                // Execute cmd3
                                execvp(cmd3[0], cmd3);
                            }
                            else
                            {
                                // Fork for cmd2
                                int status2;
                                int pid2 = fork();

                                if (pid2 == 0)
                                {
                                    // Close both pipe ends
                                    close(fd[0]);
                                    close(fd[1]);

                                    // Execute cmd2
                                    execvp(cmd2[0], cmd2);
                                }
                                else
                                {
                                    // Fork for cmd1
                                    int status;
                                    int pid = fork();

                                    if (pid == 0)
                                    {
                                        // Set the write end of the pipe to be our standard output so result of execution of command gets written to pipe
                                        dup2(fd[1], 1);

                                        // Close both pipe ends
                                        close(fd[0]);
                                        close(fd[1]);

                                        // Execute cmd1
                                        execvp(cmd1[0], cmd1);
                                    }
                                    else
                                    {

                                        // Close both pipe ends and wait for the child process to complete
                                        close(fd[0]);
                                        close(fd[1]);
                                        waitpid(pid, &status, 0);
                                    }

                                    // Close both pipe ends and wait for the child process to complete
                                    close(fd[0]);
                                    close(fd[1]);
                                    waitpid(pid2, &status2, 0);
                                }

                                // Close both pipe ends and wait for the child process to complete
                                close(fd[0]);
                                close(fd[1]);
                                waitpid(pid3, &status3, 0);
                            }

                            // Wait for child if & was used
                            if (run_background == false)
                            {
                                // Close both pipe ends
                                close(fd[0]);
                                close(fd[1]);

                                waitpid(pid4, &status4, 0);
                            }
                        }

                        // Flush the input buffer
                        fflush(stdin);

                        // Restore the stdin to the keyboard
                        dup2(original_in, 0);
                        close(original_in);
                    }
                }
            }
        }
        else if(parse_response == 2)
        {
            // exit command was entered to parser. terminate program.
            break;
            break;
        }
    }
    return 0;
}