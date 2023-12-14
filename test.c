#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096

int main() {
    int pipe_fd[2];
    pid_t child_pid;
    char buffer[BUFFER_SIZE];
    int status;

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork the process
    child_pid = fork();

    if (child_pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0)
    {
        // Child process
        close(pipe_fd[0]); // Close the read end of the pipe

        // Redirect stdout to the pipe
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // Execute the child process (replace "child_program" with your actual command)
        execlp("ls", "ls", (char *)NULL);

        // If exec fails
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        close(pipe_fd[1]); // Close the write end of the pipe

        // Wait for the child to finish
        waitpid(child_pid, &status, 0);

        // Read from the read end of the pipe into a buffer
        ssize_t bytesRead = read(pipe_fd[0], buffer, BUFFER_SIZE - 1);

        if (bytesRead == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        buffer[bytesRead] = '\0'; // Null-terminate the string

        // Print or use the captured output as needed
        printf("Child process output:\n%s", buffer);

        // Close the read end of the pipe
        close(pipe_fd[0]);
    }

    return 0;
}
