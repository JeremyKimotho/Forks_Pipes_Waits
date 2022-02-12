#include <iostream>
using namespace std;
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string>

#define B_SIZE 1024
#define DELIMITER " "

/*

    Takes input received from user and checks it for errors. If the input is valid and does not contain errors, 0 is returned. If there are errors, then 1 is returned. If the user enters the exit command "exit", then 2 is returned.

*/
int parser(char *argv[50])
{
    bool pipe_used = false;
    bool background_used = false;

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
            // code for process execution
            cout << "Valid input \n" << endl;
        }
        else if(parse_response == 2)
        {
            // exit command was entered to parser. terminate program.
            break;
        }
    }
    return 0;
}