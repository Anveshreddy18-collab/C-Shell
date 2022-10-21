#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

// Defining sizes of the input, tokens and number of tokens
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

void red()
{
    printf("\033[1;31m");
}

void yellow()
{
    printf("\033[1;33m");
}

void reset()
{
    printf("\033[0m");
}
void green()
{
    printf("\033[0;32m");
}
void purple()
{
    printf("\033[0;35m");
}

// This functions takes a line and cuts it into tokens
char **tokenize(char *line)
{
    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for (i = 0; i < strlen(line); i++)
    {

        char readChar = line[i];
        char next;
        if(i+1<strlen(line)) next=line[i+1];

        if (readChar == ' ' || readChar == '\n' || readChar == '\t')
        {
            token[tokenIndex] = '\0';
            if (tokenIndex != 0)
            {
                tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        }
        else
        {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL;
    return tokens;
}

int found_char(char * line, char c) // returns the index where c is found in line
{
    int res=-1;
    for(int i=0;i<strlen(line);i++)
    {
        if(line[i]==c) {res=i; break;}
    }
    return res;
}


void print_guide()
{
    purple();
    printf("\n-----------------------------------------------\n");
    printf("\n************** HELP DESK *************\n");
    printf("\n1) cd [path]: cd stands for change directory. Include a path after cd to change to get into that directory\n");
    printf("\n2) ls: To print all the files/directories in current directory\n");
    printf("\n3) cat, cat [file]: concatenate files and print on the standard output \n");
    printf("\n4) echo : print some sentence to stdout \n");
    printf("\n5) sleep [amount of time+unit] : sleeps for the given amount of time\n");
    printf("\n6) exit, quit: Exits out of the shell\n");
    printf("\n7) clear: Clears the output\n");
    printf("\n8) man [option]: To get manual for any command, replace the command in place of [option]\n\n");
    reset();
}
void execute_execvp(char **tokens)
{
    signal(SIGINT, SIG_IGN);
    if (strcmp(tokens[0], "exit") == 0 || strcmp(tokens[0], "quit") == 0)
    {
        pid_t parent_pid = getppid();
        kill(parent_pid, 9);
    }
    else if (!strcmp(tokens[0], "help"))
    {
        print_guide();
    }
    else if (!strcmp(tokens[0], "cd"))
    {
        exit(0);
    }
    else if (execvp(tokens[0], tokens) < 0)
    {
        fprintf(stderr, "Sorry, we don't do that here!\n");
    }
    exit(0);
}
void execute_chdir(char **tokens)
{
    char s[1024];
    // printf("Previous Path: %s\n", getcwd(s, 1024));
    // s[strlen(s)] = '/';
    // strcat(s, tokens[1]);
    if (chdir(tokens[1]) == -1)
    {
        fprintf(stderr, "Error changing to the directory. Check the path\n");
    }
    else
    {
        if (tokens[2] != NULL)
        {
            fprintf(stderr, "Too many arguments\n");
        }
        else
        {
            char s1[1024];
            purple();
            printf("Current Path: ");
            reset();
            printf("%s\n", getcwd(s1, 1024));
        }
    }
}

void pipe_it(char **tokens, char **tokens1)
{
    int p[2];
    pipe(p);
    if (fork() == 0)
    {
        close(1);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        execute_execvp(tokens);
    }
    if (fork() == 0)
    {
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        execute_execvp(tokens1);
    }
    close(p[0]);
    close(p[1]);
    wait(NULL);
    wait(NULL);
    exit(0);
}

char ** semicolon_parsing(char *line)
{
    char **commands = (char **)malloc(1000 * sizeof(char *));
    char *command = (char *)malloc(100 * sizeof(char));
    char *p = strstr(line + 0, ";;");
    int ind = 0;
    int count = 0;
    int match = p - line;
    while (p != NULL)
    {
        int j = 0;
        for (int i = ind; i < match; i++)
        {
            command[j] = line[i];
            j++;
        }
        command[j] = '\0';
        commands[count] = (char *)malloc(1000 * sizeof(char));
        strcpy(commands[count++], command);
        match += 2;
        ind = match;
        p = strstr(line + match, ";;");
        match = p - line;
    }
    if (match != strlen(line) - 2)
    {
        int j = 0;
        for (int i = ind; i < strlen(line); i++)
        {
            command[j] = line[i];
            j++;
        }
        command[j] = '\0';
        commands[count] = (char *)malloc(1000 * sizeof(char));
        strcpy(commands[count++], command);
    }
    commands[count] = NULL;
    return commands;
}

int main()
{
    char line[MAX_INPUT_SIZE];
    char **tokens;
    int i;

    while (1)
    {
        signal(SIGINT, SIG_IGN);
        red();
        char *username = getenv("USER");
        printf("@%s>", username);
        reset();
        bzero(line, MAX_INPUT_SIZE);
        gets(line);
        if(strlen(line)==0) continue;
        line[strlen(line)] = '\n'; //terminate with new line
        char **commands=semicolon_parsing(line);
        // for (int i = 0; commands[i] != NULL; i++)
        // {
        //     printf("%s\n", commands[i]);
        // }
        // exit(0);
        for (int k = 0; commands[k] != NULL; k++)
        {
            strcpy(line,commands[k]);
            int flag1= found_char(line,'>'); // returns the index where redirection is present
            int flag2= found_char(line, '|');  // returns the index where pipe is present

            // if either redirection or piping is present, we will split the command into two halves
            char line1[MAX_INPUT_SIZE]; // contains the right half
            bzero(line1, MAX_INPUT_SIZE); 
        
            if(flag1>=0 || flag2>=0)
            {
                int ind=flag1+flag2+1;
                for(int j=ind+2;j<strlen(line);j++)
                {
                    line1[j-ind-2]=line[j];
                }
                line[ind]='\0';
                line1[strlen(line1)]='\0';
            }
            
            tokens = tokenize(line);
            char **tokens1;
            tokens1=tokenize(line1);

            pid_t id=fork();
            if(id==-1)
            {
                printf("Fork failed!\n");
            }
            else if(id==0)
            {
                if(flag2>=0)
                {
                    pipe_it(tokens, tokens1);
                }
                else
                {
                    // Either redirection or just simple commands
                    int file_desc;
                    int temp=1;
                    if (flag1 >= 0) // when redirecion is there
                    {
                        file_desc = open(tokens1[0], O_CREAT | O_WRONLY);
                        temp = dup(1);      // storing a copy to STDOUT. temp now refers to STDOUT
                        dup2(file_desc, 1); // 1 now points to tokens1[0] file not STDOUT.
                        // so everything that goes to STDOUT in general, will now go to tokens1[0]
                    }
                    execute_execvp(tokens);
                    close(file_desc);
                    dup2(temp, 1);
                    close(temp);
                }
                

            }
            else
            {
                wait(NULL);
                if (!strcmp(tokens[0], "cd"))
                {
                    execute_chdir(tokens);
                }
            }

            // Freeing the allocated memory
            for (i = 0; tokens[i] != NULL; i++)
            {
                free(tokens[i]);
            }
            for (i = 0; tokens1[i] != NULL; i++)
            {
                free(tokens1[i]);
            }
            free(tokens);
            free(tokens1);
            yellow();
            printf("---------------------------------------\n");
        }
    }
    return 0;
}