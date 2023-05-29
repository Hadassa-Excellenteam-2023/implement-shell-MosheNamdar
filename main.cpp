//-------include section----------
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>



//------prototypes------------------
void executeCommand(const std::string& command);
std::vector<std::string> splitString(const std::string& input, char delimiter);


//-------main-----------------------
int main() {
	
    std::string input;

    while (true) {
        std::cout << "Shell> ";
        std::getline(std::cin, input);

        if (input == "exit")
            break;

        executeCommand(input);
    }

    return 0;
}

//----------------------functions-----------------------------
// Function to execute a command
void executeCommand(const std::string& command)
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
        std::vector<std::string> args = splitString(command, ' ');

        // Convert vector of strings to char* array
        std::vector<char*> cArgs;
        for (const auto& arg : args)
            cArgs.push_back(const_cast<char*>(arg.c_str()));
	
        cArgs.push_back(nullptr); // Add a null terminator at the end

        execvp(cArgs[0], cArgs.data());

        // execvp will only return if an error occurred
        std::cerr << "Failed to execute command: " << command << std::endl;
        exit(EXIT_FAILURE);
        
    } 
    
    else 
    {
        // Parent process
        int status;
        
        waitpid(pid, &status, 0);
        if (status != 0)
            std::cerr << "Command exited with non-zero status: " << command << std::endl;
    }
}

//--------------------------------------------------------------
// Function to split a string into tokens based on delimiter
std::vector<std::string> splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, delimiter))
        tokens.push_back(token);

    return tokens;
}


//------------------------------------------------------------------
//------------------------------------------------------------------
//------------------------------------------------------------------


