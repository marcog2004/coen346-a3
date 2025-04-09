// Marco Greco 40285114

#include <iostream>
#include <vector>
#include <fstream>
#include <string>

using namespace std;

class Process { // process class
    public: // public variables
        string processID; // associated process number (according to user)
        int startTime; // time when process is ready to execute
        int duration; // duration of process execution

        Process(string iprocessID, int istartTime, int iduration) : processID(iprocessID), startTime(istartTime), duration(iduration) {};
};
class VirtualMemoryManager{
    public:
        void store(){};
        void release(){};
        void lookup(){};
};

class Command {
    public:
        virtual void execute(VirtualMemoryManager vmm, Process process) = 0;
        virtual ~Command() {}

};

class StoreCommand : public Command {
        string variableID;
        string value;
    public:
        StoreCommand(string ivariableID, string ivalue) : variableID(ivariableID), value(ivalue) {}
        void execute(VirtualMemoryManager vmm, Process process) override {
            vmm.store();
        }
        string getVariableID() const { return variableID; }
        string getValue() const { return value; }
};

class ReleaseCommand : public Command {
        string variableID;
    public:
        ReleaseCommand(string ivariableID) : variableID(ivariableID) {}
        void execute(VirtualMemoryManager vmm, Process process) override {
            vmm.release();
        }
        string getVariableID() const { return variableID; }
};

class LookupCommand : public Command {
        string variableID;
    public:
        LookupCommand(string ivariableID) : variableID(ivariableID) {}
        void execute(VirtualMemoryManager vmm, Process process) override {
            vmm.lookup();
        }
        string getVariableID() const { return variableID; }
};

void describeCommand(Command* command) {
    if (auto store = dynamic_cast<StoreCommand*>(command)) {
        cout << "Command Type: Store" << endl;
        cout << "Variable ID: " << store->getVariableID() << endl;
        cout << "Value: " << store->getValue() << endl;
    } else if (auto release = dynamic_cast<ReleaseCommand*>(command)) {
        cout << "Command Type: Release" << endl;
        cout << "Variable ID: " << release->getVariableID() << endl;
    } else if (auto lookup = dynamic_cast<LookupCommand*>(command)) {
        cout << "Command Type: Lookup" << endl;
        cout << "Variable ID: " << lookup->getVariableID() << endl;
    } else {
        cout << "Unknown command type." << endl;
    }
}


int main(){

    // Open memconfig file
    ifstream memconfigFile("memconfig.txt");
    if(!memconfigFile){
        cout << "Error opening file" << endl;
        return 1;
    }

    // Read the number of pages from the memconfig file
    int pages;
    memconfigFile >> pages; // read number of pages
    cout << "Number of pages: " << pages << endl << endl;

    memconfigFile.close();

    // Open processes file
    ifstream processesFile("processes.txt");
    if(!processesFile){
        cout << "Error opening file" << endl;
        return 1;
    }

    // Read number of cores and number of processes from Processes file
    int C;
    processesFile >> C; // Read the number of cores
    cout << "Number of cores: " << C << endl;

    int N;
    processesFile >> N; // Read the number of processes
    cout << "Number of processes: " << N << endl << endl;

    // Extract all processes from Processes file
    int processID;
    int startTime;
    int duration;
    vector<Process> processes; // vector of processes
    for (int i = 0; i < N; i++) {
        processesFile >> startTime >> duration; // Read process data
        Process p(to_string(i+1), startTime, duration); // Create process object
        processes.push_back(p); // Add process to vector
    }

    processesFile.close();

    // Output processes with their data
    for (int i = 0; i < N; i++){
        cout << "Process ID: " << processes[i].processID << endl;
        cout << "Start Time: " << processes[i].startTime << endl;
        cout << "Duration: " << processes[i].duration << endl;
        cout << endl;
    }

    // Open commands file
    ifstream commandsFile("commands.txt");
    if(!commandsFile){
        cout << "Error opening file" << endl;
        return 1;
    }

    string commandType;
    string variableID;
    string value;

    vector<Command*> commands;

    while (commandsFile >> commandType){
        if (commandType == "Store"){
            commandsFile >> variableID >> value;
            commands.push_back(new StoreCommand(variableID, value)); // Create a new StoreCommand object and add it to the vector
        }else if (commandType == "Lookup"){
            commandsFile >> variableID;
            commands.push_back(new LookupCommand(variableID));
        }else if (commandType == "Release"){
            commandsFile >> variableID;
            ReleaseCommand releaseCommand(variableID);
            commands.push_back(new ReleaseCommand(variableID)); // Create a new ReleaseCommand object and add it to the vector
        }else{
            cout << "Unknown command: " << commandType << endl;
        }
    }

    for (auto cmd : commands) {
        describeCommand(cmd);
        cout << endl;
    }

    for (auto cmd : commands) {
        delete cmd;
    }
}