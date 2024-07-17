#include <fstream>           // Include file stream library for file handling
#include <iostream>          // Include iostream library for input and output operations
#include <sys/inotify.h>     // Include inotify library for file system event monitoring
#include <thread>            // Include thread library for multithreading
#include <nlohmann/json.hpp> // Include JSON library for JSON operations
#include <signal.h>          // Include signal handling library
#include <dirent.h>          // Include directory entry library for directory operations

std::string config_path; // Declare a string to store the configuration file path
nlohmann::json config;   // Declare a JSON object to store configuration data

// Function to load configuration from the config file
void loadConfig()
{
    try
    {
        std::ifstream config_file(config_path); // Open the configuration file
        if (config_file.is_open())
        {
            config_file >> config; // Read JSON data from the file into the config object
            std::cout << "Loaded config" << std::endl;
            config_file.close(); // Close the configuration file
        }
    }
    catch (nlohmann::json::parse_error &e)
    {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
}

// Abstract base class for process management
class Core
{
protected:
    virtual int startProcess(nlohmann::json process) = 0;              // Pure virtual function to start a process
    virtual bool isProcessRunning(const std::string &processName) = 0; // Pure virtual function to check if a process is running

    // Granting access to ProcessManager
    friend class ProcessManager;
};

// Derived class for Linux process management
class LinuxProcessManager : public Core
{
protected:
    // Function to start a process on Linux
    int startProcess(nlohmann::json process) override
    {
        std::string appPath = process["path"]; // Get the application path from the JSON object
        nlohmann::json args = process["args"]; // Get the arguments from the JSON object

        pid_t pid = fork(); // Fork the process
        std::cout << "pid: " << pid << std::endl;
        if (pid == 0)
        {
            std::vector<char *> argv; // Create a vector to store arguments
            argv.push_back(const_cast<char *>(appPath.c_str()));

            if (!args.empty())
            {
                for (const auto &arg : args)
                {
                    argv.push_back(const_cast<char *>(arg.get<std::string>().c_str()));
                }
            }
            argv.push_back(nullptr);             // Add null terminator for execv
            execv(appPath.c_str(), argv.data()); // Execute the process
            perror("execv");
        }
        else if (pid < 0)
        {
            std::cerr << "Failed to start process: " << appPath << std::endl;
            return -1;
        }
        return pid; // Return the process ID
    }

    // Function to check if a process is running on Linux
    bool isProcessRunning(const std::string &processName) override
    {
        DIR *dir;
        struct dirent *entry;
        bool found = false;
        std::string pid;

        dir = opendir("/proc"); // Open the /proc directory
        if (dir == NULL)
        {
            perror("opendir failed");
            return false;
        }

        while ((entry = readdir(dir)) != NULL)
        {
            // Check if the entry is a directory and its name is a number (PID)
            if (entry->d_type == DT_DIR && std::isdigit(entry->d_name[0]))
            {
                pid = entry->d_name;
                std::string cmdlinePath = "/proc/" + pid + "/cmdline";
                std::ifstream cmdlineFile(cmdlinePath.c_str());
                if (cmdlineFile.is_open())
                {
                    std::string cmdline;
                    std::getline(cmdlineFile, cmdline);
                    // Check if the process name is found in the command line of the process
                    if (cmdline.find(processName) != std::string::npos)
                    {
                        found = true;
                        break;
                    }
                }
                cmdlineFile.close();
            }
        }

        closedir(dir); // Close the /proc directory
        return found;  // Return whether the process is running
    }
};

// Derived class for Windows process management (needs implementation)
class WindowsProcessManager : public Core
{
protected:
    // Function to start a process on Windows (not implemented)
    int startProcess(nlohmann::json process) override
    {
        // Need to be implemented
        return -1;
    }

    // Function to check if a process is running on Windows (not implemented)
    bool isProcessRunning(const std::string &processName) override
    {
        // Need to be implemented
        return false;
    }
};

// Class to manage processes
class ProcessManager
{
private:
    std::unique_ptr<Core> core; // Unique pointer to a Core object

    // Function to reload configuration and start processes
    void reloadConfig()
    {
        loadConfig();     // Load the configuration
        startProcesses(); // Start the processes based on the configuration
    }

public:
    // Constructor to initialize ProcessManager based on the operating system
    ProcessManager(std::string os)
    {
        if (os == "linux")
            core = std::make_unique<LinuxProcessManager>();
        else if (os == "windows")
            core = std::make_unique<WindowsProcessManager>();
    }

    // Function to start processes based on the configuration
    void startProcesses()
    {
        nlohmann::json processes = config["processes"]; // Get the processes from the configuration
        std::cout << processes << std::endl;

        for (auto &process : processes)
        {
            if (!core->isProcessRunning(process["name"]))
            {
                int pid = core->startProcess(process); // Start the process if it's not already running
                std::cout << pid << std::endl;
            }
        }
    }

    // Function to handle configuration changes
    void handleConfigChange()
    {
        std::cout << "changing..." << std::endl;

        int fd = inotify_init(); // Initialize inotify
        if (fd < 0)
            return;

        int wd = inotify_add_watch(fd, config_path.c_str(), IN_MODIFY); // Add watch for modifications
        if (wd < 0)
            return;

        char buffer[1024];
        while (true)
        {
            int length = read(fd, buffer, 1024); // Read events from inotify
            if (length < 0)
                continue;

            std::cout << "changed" << std::endl;
            std::cout << buffer << std::endl;
            reloadConfig(); // Reload the configuration if a change is detected
        }

        inotify_rm_watch(fd, wd); // Remove the watch
        close(fd);                // Close inotify
    }
};

// Main function
int main()
{
    config_path = "config.json"; // Set the configuration file path
    loadConfig();                // Load the configuration

    ProcessManager pm("linux"); // Create a ProcessManager for Linux
    pm.startProcesses();        // Start processes based on the configuration

    std::thread handleConfigChangeThread(&ProcessManager::handleConfigChange, &pm); // Start a thread to handle configuration changes
    handleConfigChangeThread.join();                                                // Wait for the configuration change thread to finish

    return 0;
}
