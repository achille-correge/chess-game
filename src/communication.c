#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

void create_pipes(int pipe_main_to_child[2], int pipe_child_to_main[2])
{
    if (pipe(pipe_main_to_child) == -1 || pipe(pipe_child_to_main) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}

void setup_child_process(int pipe_main_to_child[2], int pipe_child_to_main[2], const char *program_name)
{
    close(pipe_main_to_child[1]); // Close write end of the first pipe
    close(pipe_child_to_main[0]); // Close read end of the second pipe

    dup2(pipe_main_to_child[0], STDIN_FILENO);  // Redirect stdin to read end of the first pipe
    dup2(pipe_child_to_main[1], STDOUT_FILENO); // Redirect stdout to write end of the second pipe

    close(pipe_main_to_child[0]); // Close the read end of the first pipe
    close(pipe_child_to_main[1]); // Close the write end of the second pipe

    execl(program_name, program_name, NULL);
    perror("execl");
    exit(EXIT_FAILURE);
}

char *communicate_with_child(int pipe_main_to_child[2], int pipe_child_to_main[2], const char *message, char answer[256], int expects_answer)
{
    write(pipe_main_to_child[1], message, strlen(message));

    // Read response from child after each message
    char buffer[256] = {0};
    if (expects_answer)
    {
        ssize_t bytes_read = read(pipe_child_to_main[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0'; // Null-terminate the string
        }
        // printf("Received response from child: %s", buffer);
    }

    strcpy(answer, buffer);

    return answer;
}

SubProcess empty_process()
{
    SubProcess sp;
    sp.write_pipe[0] = -1;
    sp.write_pipe[1] = -1;
    sp.read_pipe[0] = -1;
    sp.read_pipe[1] = -1;
    sp.pid = -1;
    return sp;
}

SubProcess gen_process()
{
    int pipe_main_to_child[2];
    int pipe_child_to_main[2];

    create_pipes(pipe_main_to_child, pipe_child_to_main);

    pid_t pid1 = fork();
    if (pid1 == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0)
    {
        setup_child_process(pipe_main_to_child, pipe_child_to_main, "./engines/alphabeta_2.0");
        printf("gen_process\n");
    }
    else
    {

        close(pipe_main_to_child[0]); // Close read end of the first pipe for each child
        close(pipe_child_to_main[1]); // Close write end of the second pipe for each child

        SubProcess sp;
        sp.write_pipe[0] = pipe_main_to_child[0];
        sp.write_pipe[1] = pipe_main_to_child[1];
        sp.read_pipe[0] = pipe_child_to_main[0];
        sp.read_pipe[1] = pipe_child_to_main[1];
        sp.pid = pid1;
        return sp;
    }
    return empty_process();
}