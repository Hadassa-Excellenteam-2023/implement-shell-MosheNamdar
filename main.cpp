/*
 * writen by: Moshe Namdar id: 208782821
 * 
 * Compile: g++ -Wall -o "main" "main.cpp"
 * Run: ./main
 */

//-------include section----------
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>


//------structs------------------
// Struct to hold information about a background process
struct BackgroundProcess {
    pid_t pid;
    std::string command;
};

// Struct to store information about a single command in a pipeline
struct PipeCommand {
    std::string command;
    bool runInBackground;
};


//------prototypes------------------
void executeCommand(const std::string& command, bool runInBackground, std::vector<BackgroundProcess>& backgroundProcesses);
std::vector<std::string> splitString(const std::string& input, char delimiter);
void showBackgroundProcesses(std::vector<BackgroundProcess>& backgroundProcesses);
void executePipeline(const std::vector<PipeCommand>& commands, std::vector<BackgroundProcess>& backgroundProcesses);


//-------main-----------------------
int main() {
    std::string input;
    std::vector<BackgroundProcess> backgroundProcesses;

    while (true)
    {
        std::cout << "Shell> ";
        std::getline(std::cin, input);

        if (input == "exit")
            break;

        if (input == "myjobs")
        {
            showBackgroundProcesses(backgroundProcesses);
            continue;
        }

        std::vector<PipeCommand> pipeCommands;

        // Split the input by pipe symbol '|'
        std::vector<std::string> commands = splitString(input, '|');
        for (const auto& cmd : commands) {
            std::string trimmedCmd = cmd;
            if (!trimmedCmd.empty() && trimmedCmd.front() == ' ')
                trimmedCmd = trimmedCmd.substr(1);
            if (!trimmedCmd.empty() && trimmedCmd.back() == ' ')
                trimmedCmd.pop_back();
            bool runInBackground = false;
            if (!trimmedCmd.empty() && trimmedCmd.back() == '&') {
                runInBackground = true;
                trimmedCmd.pop_back();
            }
            pipeCommands.push_back({ trimmedCmd, runInBackground });
        }

        executePipeline(pipeCommands, backgroundProcesses);
    }

    return 0;
}


//----------------------functions-----------------------------
// Function to execute a single command
void executeCommand(const std::string& command, bool runInBackground, std::vector<BackgroundProcess>& backgroundProcesses)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        std::cerr << "Failed to create child process" << std::endl;
        return;
    }

    else if (pid == 0)
    {
		// Child process

        // Split command into arguments
        std::vector<std::string> args = splitString(command, ' ');

        bool inputRedirect = false;
        bool outputRedirect = false;
        std::string inputFile, outputFile;
        std::vector<char*> cArgs;

		// Process command arguments
        for (const auto& arg : args)
        {
            if (arg == "<")
            {
                inputRedirect = true;
                continue;
            }

            if (arg == ">")
            {
                outputRedirect = true;
                continue;
            }

            if (inputRedirect)
            {
                inputFile = arg;
                inputRedirect = false;
                continue;
            }

            if (outputRedirect)
            {
                outputFile = arg;
                outputRedirect = false;
                continue;
            }

            cArgs.push_back(const_cast<char*>(arg.c_str()));
        }

        cArgs.push_back(nullptr);

		// Handle input redirection
        if (!inputFile.empty())
        {
            int inputFileDescriptor = open(inputFile.c_str(), O_RDONLY);
            if (inputFileDescriptor == -1)
            {
                std::cerr << "Failed to open input file: " << inputFile << std::endl;
                exit(EXIT_FAILURE);
            }

            dup2(inputFileDescriptor, STDIN_FILENO);
            close(inputFileDescriptor);
        }

		// Handle output redirection
        if (!outputFile.empty())
        {
            int outputFileDescriptor = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (outputFileDescriptor == -1)
            {
                std::cerr << "Failed to open output file: " << outputFile << std::endl;
                exit(EXIT_FAILURE);
            }

            dup2(outputFileDescriptor, STDOUT_FILENO);
            close(outputFileDescriptor);
        }

		// Execute the command
        execvp(cArgs[0], cArgs.data());

        std::cerr << "Failed to execute command: " << command << std::endl;
        exit(EXIT_FAILURE);
    }

    else
    {
		// Parent process
        if (runInBackground)
        {
			// Add background process information
            BackgroundProcess process;
            process.pid = pid;
            process.command = command;
            backgroundProcesses.push_back(process);
            std::cout << "Background process started: " << command << std::endl;
        }

        else
        {
			// Wait for the command to finish
            int status;
            waitpid(pid, &status, 0);

            if (status != 0)
                std::cerr << "Command exited with non-zero status: " << command << std::endl;
        }
    }
}

//------------------------------------------------------------
// Function to split a string into tokens based on a delimiter
std::vector<std::string> splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);

    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);

    return tokens;
}

//------------------------------------------------------------
// Function to display information about background processes
void showBackgroundProcesses(std::vector<BackgroundProcess>& backgroundProcesses) {
    std::cout << "Background processes:" << std::endl;

    auto it = backgroundProcesses.begin();
    while (it != backgroundProcesses.end()) {
        int status;
        pid_t result = waitpid(it->pid, &status, WNOHANG);
        if (result == -1 || result > 0) {
            it = backgroundProcesses.erase(it);
        } else {
            std::cout << "PID: " << it->pid << ", Command: " << it->command << std::endl;
            ++it;
        }
    }
}

//------------------------------------------------------------
// Function to execute a pipeline of commands
void executePipeline(const std::vector<PipeCommand>& commands, std::vector<BackgroundProcess>& backgroundProcesses) {
    std::vector<int> pipes(commands.size() - 1);  // Create pipes for communication

    for (size_t i = 0; i < commands.size(); ++i) {
        int pipeFD[2];
        if (i < commands.size() - 1) {
            if (pipe(pipeFD) == -1) {
                std::cerr << "Failed to create pipe" << std::endl;
                return;
            }
        }

        pid_t pid = fork();

        if (pid == -1) {
            std::cerr << "Failed to create child process" << std::endl;
            return;
        }

        else if (pid == 0) {
            // Child process
            if (i > 0) {
                dup2(pipes[i - 1], STDIN_FILENO);  // Redirect input from previous pipe
                close(pipes[i - 1]);  // Close the read end of the previous pipe
            }

            if (i < commands.size() - 1) {
                dup2(pipeFD[1], STDOUT_FILENO);  // Redirect output to the next pipe
                close(pipeFD[0]);  // Close the read end of the current pipe
                close(pipeFD[1]);  // Close the write end of the current pipe
            }

            executeCommand(commands[i].command, commands[i].runInBackground, backgroundProcesses);

            exit(EXIT_SUCCESS);
        }

        else {
            // Parent process
            if (i > 0)
                close(pipes[i - 1]);  // Close the read end of the previous pipe

            if (i < commands.size() - 1)
                pipes[i] = pipeFD[0];  // Store the read end of the current pipe for the next command

            if (!commands[i].runInBackground) {
                int status;
                waitpid(pid, &status, 0);
                if (status != 0)
                    std::cerr << "Command exited with non-zero status: " << commands[i].command << std::endl;
            }
        }
    }

    // Close all remaining pipe file descriptors in the parent process
    for (int pipeFD : pipes)
        close(pipeFD);
}


//-------------------------------------------------------
//-------------------------------------------------------
//-------------------------------------------------------
