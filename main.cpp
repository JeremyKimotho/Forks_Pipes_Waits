#include <iostream>
using namespace std;
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string>
#include <cstring>
#include <fcntl.h>

#define B_SIZE 1024
#define DELIMITER " "

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
            if (x != 4)
            {
                cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                     << endl;
                return 1;
            }
        }
        else if (special_pipe_index == 2)
        {
            if (x != 4 && x != 5)
            {
                cout << "Error: $ command used in incorrect format. Use correct formats (cmd1 $ cmd2 cmd3), (cmd1 cmd2 $ cmd3), (cmd1 cmd2 $ cmd3 cmd4) ! \n"
                     << endl;
                return 1;
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

        // i = 0;
        // while(argv[i] != NULL)
        // {
        //     printf("%s \n", argv[i]);
        //     i++;
        // }

        int parse_response = parser(argv);
        if(parse_response == 0)
        {
            bool run_background = false;

            if (strcmp(argv[i - 1],"&") == 0)
            {
                run_background = true;
                argv[i - 1] = NULL;
                i--;
            } 

            int operator_response = checkOperators(argv);

            if (operator_response == 0)
            {
                int status;
                int pid = fork();

                if (pid == 0)
                {
                    cout << endl;
                    execvp(argv[0], argv);
                }

                wait(&status);
            }
            else if (operator_response == 1)
            {
                cout << "Valid input: | operator, background " << run_background << endl;
            }
            else if (operator_response == 2)
            {
                cout << "Valid input: < operator, background " << run_background << endl;
            }
            else if (operator_response == 3)
            {
                cout << "Valid input: > operator, background " << run_background << endl;
            }
            else if (operator_response == 4)
            {
                cout << "Valid input: $ operator, background " << run_background << endl;
            }

        }
        else if(parse_response == 2)
        {
            // exit command was entered to parser. terminate program.
            break;
        }
    }
    return 0;
}