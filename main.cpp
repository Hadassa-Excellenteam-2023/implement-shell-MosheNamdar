/*
 * writen by: Moshe Namdar id: 208782821
 * 
 * The code is a simple shell program that allows users to enter commands.
 * It supports running commands in the foreground or background by using the '&' symbol.
 * The code utilizes fork() and execvp() to create child processes and execute the entered commands.
 * It also keeps track of background processes and provides a "myjobs" command to list them.
 * The program continues to prompt for commands until the user enters "exit".
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

// Struct to hold information about a background process
struct BackgroundProcess {
    pid_t pid;                // Process ID
    std::string command;     // Command executed by the process
};

//------prototypes------------------
void executeCommand(const std::string& command, bool runInBackground, std::vector<BackgroundProcess>& backgroundProcesses);
std::vector<std::string> splitString(const std::string& input, char delimiter);
void showBackgroundProcesses(const std::vector<BackgroundProcess>& backgroundProcesses);


//-------main-----------------------
int main() {
    std::string input;        // User input
    std::vector<BackgroundProcess> backgroundProcesses;      // List of background processes

    while (true)
    {
        std::cout << "Shell> ";
        std::getline(std::cin, input);     //Get user input

        if (input == "exit")
            break;

        if (input == "myjobs")
        {
            showBackgroundProcesses(backgroundProcesses);
            continue;
        }

        // Check if the command should be run in the background
        bool runInBackground = false;
        if (!input.empty() && input.back() == '&')
        {
            runInBackground = true;
            input.pop_back();
        }

        executeCommand(input, runInBackground, backgroundProcesses);
    }

    return 0;
}

//----------------------functions-----------------------------
void executeCommand(const std::string& command, bool runInBackground, std::vector<BackgroundProcess>& backgroundProcesses)
{
    pid_t pid = fork();     // Create a child process
    
    if (pid == -1)
    {
        std::cerr << "Failed to create child process" << std::endl;
        return;
    } 
    
    else if (pid == 0)
    {
        // Child process
        std::vector<std::string> args = splitString(command, ' ');

        // Convert vector of strings to char* array
        std::vector<char*> cArgs;
        for (const auto& arg : args)
            cArgs.push_back(const_cast<char*>(arg.c_str()));
        
        cArgs.push_back(nullptr); // Add a null terminator at the end

        execvp(cArgs[0], cArgs.data());       // Execute the command

        // execvp will only return if an error occurred
        std::cerr << "Failed to execute command: " << command << std::endl;
        exit(EXIT_FAILURE);
    }
    
    else
    {
        // Parent process
        if (runInBackground)
        {
            BackgroundProcess process;
            process.pid = pid;
            process.command = command;
            backgroundProcesses.push_back(process);    // Add the background process to the list
            std::cout << "Background process started: " << command << std::endl;
        }
        
        else
        {
            int status;
            waitpid(pid, &status, 0);       // Wait for the child process to finish
            
            if (status != 0)
                std::cerr << "Command exited with non-zero status: " << command << std::endl;
        }
    }
}


//--------------------------------------------------------------
std::vector<std::string> splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> tokens;       // Vector to store the split strings
    std::string token;
    std::istringstream tokenStream(input);
    
    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);         // Add each token to the vector
    
    return tokens;
}


//--------------------------------------------------------------
void showBackgroundProcesses(const std::vector<BackgroundProcess>& backgroundProcesses)
{
    std::cout << "Background processes:" << std::endl;
    
    for (const auto& process : backgroundProcesses)
        std::cout << "PID: " << process.pid << ", Command: " << process.command << std::endl;       // Display each background process
}


//------------------------------------------------------------------
//------------------------------------------------------------------
//------------------------------------------------------------------
