// Marco Greco 40285114
// Julian Valencia 40244362
// Talal Hammami 40273059

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <mutex>
#include <map>
#include <thread>
#include <random>
#include <algorithm>
#include <condition_variable>

using namespace std;

class Process { // process class
public: // public variables
    string processID; // associated process number (according to user)
    double startTime; // time when process is ready to execute
    double duration; // duration of process execution

    Process(string iprocessID, double istartTime, double iduration) : processID(iprocessID), startTime(istartTime), duration(iduration) {}; // constructor
};

double clk = 0; //global clock
int sharedCommandIndex = 0;
mutex commandIndexMtx;
mutex clkMtx; //mutex for the clock
mutex logMtx; //mutex for log times
ofstream output("output.txt");

void clockThread() {
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(1));
        lock_guard<mutex> lock(clkMtx);
        clk += 10;
    }
}

struct Page { //struct for memory slots, each page is a slot
    string variableID;
    string value;
    double lastTimeAccessed = -1;
    bool empty = true;
};

class VirtualMemoryManager {
    int mainMemorySize; //max number of pages
    vector<Page> mainMemory; //memory block
    map<string, Page> disk; //disk storage
    mutex memoryMutex; //synchronize memory

public:
    VirtualMemoryManager(int size) : mainMemorySize(size), mainMemory(size) {
        ofstream diskFile("vm.txt", ios:: trunc);
    }

    void store(string variableID, string value) {
        lock_guard<mutex> lock(memoryMutex); //lock mutex
        for (auto& page : mainMemory) { //if page exists in memory, update its value and access time
            if (!page.empty && page.variableID == variableID) {
                page.value = value;
                page.lastTimeAccessed = clk;
                return;
            }
        }
        for (auto& page : mainMemory) { //if page is empty, store new page there
            if (page.empty) {
                page = { variableID, value, clk, false };
                return;
            }
        }
        disk[variableID] = { variableID, value, clk, false }; //store page in disk if memory is full
        write(variableID, value);
    }

    void release(string variableID) {
        lock_guard<mutex> lock(memoryMutex);
        for (auto& page : mainMemory) { //search mainMemory and disk for ID
            if (!page.empty && page.variableID == variableID) {
                page = Page(); //clears the page in memory
                return;
            }
        }
        disk.erase(variableID); //erases it from memory
        remove(variableID);
    }

    string lookup(string variableID) {
        lock_guard<mutex> lock(memoryMutex);
        for (auto& page : mainMemory) { //check mainMemory for page and update last accessed time and returns value
            if (!page.empty && page.variableID == variableID) {
                page.lastTimeAccessed = clk;
                return page.value;
            }
        }

        string diskValue = read(variableID);
        if (diskValue != "-1") { //checks disk if not found in memory
            Page diskPage = disk[variableID];
            int index = 0;
            double earliestTime = clk;
            for (int i = 0; i < mainMemorySize; ++i) {  //chooses page in memory
                if (mainMemory[i].lastTimeAccessed < earliestTime) {
                    earliestTime = mainMemory[i].lastTimeAccessed;
                    index = i;
                }
            }
            output << "Clock: " << clk << ", Memory Manager, SWAP: Variable "
                << diskPage.variableID << " with Variable " << mainMemory[index].variableID << endl;

            cout << "Clock: " << clk << ", Memory Manager, SWAP: Variable "
                << diskPage.variableID << " with Variable " << mainMemory[index].variableID << endl;

                write(mainMemory[index].variableID, mainMemory[index].value);
                mainMemory[index] = { variableID, diskValue, clk, false };
                remove(variableID);
                return diskValue;
        }
        return "-1"; //not found
    }

private:
    void write(string variableID, string value){ //writes to file
        ifstream inFile("vm.txt");
        vector<pair<string, string>> diskData;
        string id, val;
        bool exists = false;

        while (inFile >> id >> val){
            if (id == variableID){
                val = value;
                exists = true;
            }
            diskData.emplace_back(id, val);
        }
        inFile.close();
        
        if (!exists){
            ofstream outFile("vm.txt", ios::app);
            outFile << variableID << " " << value << endl;
            outFile.close();
        }
    }

    string read(const string& variableID){ //reads from file
        ifstream inFile("vm.txt");
        string id, val;
        while (inFile >> id>> val){
            if (id == variableID){
                inFile.close();
                return val;
            }
        }
        inFile.close();
        return "-1";
    }

    void remove(const string& variableID){ //removes from file
        ifstream inFile("vm.txt");
        vector<pair<string, string>> diskData;
        string id, val;

        while (inFile >> id >> val){
            if (id != variableID){
                diskData.emplace_back(id, val);
            }
        }
        inFile.close();

        ofstream outFile("vm.txt", ios::trunc);
        for (const auto& [id, val] : diskData){
            outFile << id << " " << val << endl;
        }
        outFile.close();
    }
};

class Command { // command abstract class
public:
    virtual void execute(VirtualMemoryManager& vmm, Process process) = 0;
};

class StoreCommand : public Command { // subclass of command
    string variableID; //variable id string
    string value; // value string
public:
    StoreCommand(string ivariableID, string ivalue) : variableID(ivariableID), value(ivalue) {} // constructor
    void execute(VirtualMemoryManager& vmm, Process process) override { // execute command runs the vmm function for store command
        vmm.store(variableID, value);
    }
    string getVariableID() const { return variableID; } // id getter
    string getValue() const { return value; } // value getter
};

class ReleaseCommand : public Command { // subclass of command
    string variableID; // variable id string
public:
    ReleaseCommand(string ivariableID) : variableID(ivariableID) {} // constructor
    void execute(VirtualMemoryManager& vmm, Process process) override { // execute command runs the vmm function for release
        vmm.release(variableID);
    }
    string getVariableID() const { return variableID; } // id getter
};

class LookupCommand : public Command { // subclass of command
    string variableID; //variable id string
public:
    LookupCommand(string ivariableID) : variableID(ivariableID) {} // constructor
    void execute(VirtualMemoryManager& vmm, Process process) override { // execute command runs the vmm function for lookup
        vmm.lookup(variableID);
    }
    string getVariableID() const { return variableID; } // id getter
};

void describeCommand(Command* command) { // function to output all commands and their characteristics
    if (auto store = dynamic_cast<StoreCommand*>(command)) { // if command is a store command
        cout << "Command Type: Store" << endl;
        cout << "Variable ID: " << store->getVariableID() << endl;
        cout << "Value: " << store->getValue() << endl;
    }
    else if (auto release = dynamic_cast<ReleaseCommand*>(command)) { // if command is release command
        cout << "Command Type: Release" << endl;
        cout << "Variable ID: " << release->getVariableID() << endl;
    }
    else if (auto lookup = dynamic_cast<LookupCommand*>(command)) { // if command is lookup
        cout << "Command Type: Lookup" << endl;
        cout << "Variable ID: " << lookup->getVariableID() << endl;
    }
    else { // check for unknown command (error)
        cout << "Unknown command type." << endl;
    }
}

void runProcess(VirtualMemoryManager& vmm, Process process, vector<Command*>& commands) {
    default_random_engine generator(random_device{}());
    uniform_int_distribution<int> distribution(1, 1000); //we want a value between 1 and 1000

    // wait until process start time
    while (true) {
        {
            lock_guard<mutex> clkLock(clkMtx);
            if (clk >= process.startTime) break;
        }
        this_thread::sleep_for(chrono::milliseconds(1));
    }

    {
        lock_guard<mutex> logLock(logMtx);
        lock_guard<mutex> clkLock(clkMtx);
        output << "Clock: " << clk << ", Process " << process.processID << ": Started." << endl;
        cout << "Clock: " << clk << ", Process " << process.processID << ": Started." << endl;
    }

    double endTime = process.startTime + process.duration;

    while (true) {
        {
            lock_guard<mutex> clkLock(clkMtx);
            if (clk >= endTime) break;
        }

        // next command is picked based on the sharedCommandIndex
        int commandIndex;
        {
            lock_guard<mutex> lock(commandIndexMtx);
            commandIndex = sharedCommandIndex;
            sharedCommandIndex = (sharedCommandIndex + 1) % commands.size();
        }

        Command* cmd = commands[commandIndex];

        // log every command called
        if (auto store = dynamic_cast<StoreCommand*>(cmd)) {
            {
                lock_guard<mutex> logLock(logMtx);
                lock_guard<mutex> clkLock(clkMtx);
                output << "Clock: " << clk << ", Process " << process.processID
                    << ", Store: Variable " << store->getVariableID()
                    << ", Value: " << store->getValue() << endl;
                cout << "Clock: " << clk << ", Process " << process.processID
                    << ", Store: Variable " << store->getVariableID()
                    << ", Value: " << store->getValue() << endl;
            }
        }
        else if (auto release = dynamic_cast<ReleaseCommand*>(cmd)) {
            {
                lock_guard<mutex> logLock(logMtx);
                lock_guard<mutex> clkLock(clkMtx);
                output << "Clock: " << clk << ", Process " << process.processID
                    << ", Release: Variable " << release->getVariableID() << endl;
                cout << "Clock: " << clk << ", Process " << process.processID
                    << ", Release: Variable " << release->getVariableID() << endl;
            }
        }
        else if (auto lookup = dynamic_cast<LookupCommand*>(cmd)) {
            string result = vmm.lookup(lookup->getVariableID());
            {
                lock_guard<mutex> logLock(logMtx);
                lock_guard<mutex> clkLock(clkMtx);
                output << "Clock: " << clk << ", Process " << process.processID
                    << ", Lookup: Variable " << lookup->getVariableID()
                    << ", Value: " << result << endl;
                cout << "Clock: " << clk << ", Process " << process.processID
                    << ", Lookup: Variable " << lookup->getVariableID()
                    << ", Value: " << result << endl;
            }
            int delay = distribution(generator);

            double currentClk;
            {
                lock_guard<mutex> clkLock(clkMtx);
                currentClk = clk;
            }

            double timeLeft = process.startTime + process.duration - currentClk;

            if (delay > timeLeft)
                delay = static_cast<int>(timeLeft);

            if (delay > 0)
                this_thread::sleep_for(chrono::milliseconds(delay));
            continue;
        }

        cmd->execute(vmm, process);

        // generates a value between 1 to 1000 to simulate waiting command times (API Calls)
        int delay = distribution(generator);

        double currentClk;
        {
            lock_guard<mutex> clkLock(clkMtx);
            currentClk = clk;
        }

        double timeLeft = process.startTime + process.duration - currentClk;

        if (delay > timeLeft)
            delay = static_cast<int>(timeLeft);

        if (delay > 0)
            this_thread::sleep_for(chrono::milliseconds(delay));
    }

    {
        lock_guard<mutex> logLock(logMtx);
        lock_guard<mutex> clkLock(clkMtx);
        output << "Clock: " << clk << ", Process " << process.processID << ": Finished." << endl;
        cout << "Clock: " << clk << ", Process " << process.processID << ": Finished." << endl;
    }
}

int main() {

    // Open memconfig file
    ifstream memconfigFile("memconfig.txt");
    if (!memconfigFile) {
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
    if (!processesFile) {
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
    double startTime;
    double duration;
    vector<Process> processes; // vector of processes
    for (int i = 0; i < N; i++) {
        processesFile >> startTime >> duration; // Read process data
        Process p(to_string(i + 1), startTime*1000, duration*1000); // Create process object
        processes.push_back(p); // Add process to vector
    }

    processesFile.close(); // close processes file

    // Output processes with their data
    for (int i = 0; i < N; i++) {
        cout << "Process ID: " << processes[i].processID << endl;
        cout << "Start Time: " << processes[i].startTime << endl;
        cout << "Duration: " << processes[i].duration << endl;
        cout << endl;
    }

    // Open commands file
    ifstream commandsFile("commands.txt");
    if (!commandsFile) {
        cout << "Error opening file" << endl;
        return 1;
    }

    string commandType;
    string variableID;
    string value;

    vector<Command*> commands; // vector of all commands (including all 3 types)

    while (commandsFile >> commandType) { // read through commands file
        if (commandType == "Store") { // if type is store, run appropriate contructor and add to vector
            commandsFile >> variableID >> value;
            commands.push_back(new StoreCommand(variableID, value)); // Create a new StoreCommand object and add it to the vector
        }
        else if (commandType == "Lookup") { // if type is lookup, run appropriate contructor and add to vector
            commandsFile >> variableID;
            commands.push_back(new LookupCommand(variableID));
        }
        else if (commandType == "Release") { // if type is release, run appropriate contructor and add to vector
            commandsFile >> variableID;
            ReleaseCommand releaseCommand(variableID);
            commands.push_back(new ReleaseCommand(variableID)); // Create a new ReleaseCommand object and add it to the vector
        } 
        else { // handle error in case of unknown command
            cout << "Unknown command: " << commandType << endl;
        }
    }

    for (auto cmd : commands) { // describe each command individually
        describeCommand(cmd);
        cout << endl;
    }

    VirtualMemoryManager vmm(pages);
    thread clkThread(clockThread);

    // FIFO scheduling, sort by arrival time (first come first serve)
    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.startTime < b.startTime;
        });

    int cores = C;
    mutex coreMtx;
    condition_variable coreCv;

    vector<thread> threads;

    for (auto& process : processes) {
        threads.emplace_back([&vmm, &commands, &process, &cores, &coreMtx, &coreCv]() {
            // if the start time has not been reached, make the thread sleep
            while (true) {
                double currentClk;
                {
                    lock_guard<mutex> clkLock(clkMtx);
                    currentClk = clk;
                }
                if (currentClk >= process.startTime)
                    break;

                this_thread::sleep_for(chrono::milliseconds(1));
            }

            {
                unique_lock<mutex> lock(coreMtx);
                coreCv.wait(lock, [&]() { return cores > 0; }); //check if available cores > 0, if not then sleep
                cores--;  // occupy a core
            }

            // run process after all checks are made
            runProcess(vmm, process, commands);

            // when process finishes, we can free up a space in the core
            {
                lock_guard<mutex> lock(coreMtx);
                cores++;
            }
            coreCv.notify_one();  // notify waiting threads
            });
    }

    // all process need to finish
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }

    clkThread.detach();
}
