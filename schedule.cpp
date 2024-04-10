#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <map>
using namespace std;
// Create our process structure, which will hold the process ID, arrival time, and service time
struct Process {
    string pid;
    int arrivalTime;
    int serviceTime;
};
// helper struct for logical comparisons, mostly SRT and SRN
struct CompareServiceTime {
    bool operator()(const Process& a, const Process& b) {
        return a.serviceTime > b.serviceTime;
    }
};
// Helper function to sort by arrival time
void sortByArrivalTime(vector<Process>& processes) {
    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.arrivalTime < b.arrivalTime;
    });
}
// Helper function to sort by service time
bool sortByServiceTime(vector<Process>& processes) {
    sort(processes.begin(), processes.end(), [](const Process& a, const Process& b) {
        return a.serviceTime < b.serviceTime;
    });
}
// Helper function to check if all queues are empty (For Feedback algorithm)
bool allQueuesEmpty(const vector<queue<Process>>& queues) {
    for(const auto& q : queues) {
        if(!q.empty()) {
            return false;
        }
    }
    return true;
}
// Function prototypes for testing
void firstComeFirstServe(vector<Process>& processes, ofstream& fcfs);
void roundRobin(vector<Process>& processes, int quantum, ofstream& rr10, ofstream& rr40);
void shortestProcessNext(vector<Process>& processes, ofstream& spn);
void shortestRemainingTime(vector<Process>& processes, ofstream& srt);
void highestResponseRatioNext(vector<Process>& processes, ofstream& hrrn);
void feedBackScheduler(vector<Process>& processes, int quantum, ofstream& feedback);
// FCFS scheduling algorithm. Complete, Works.
void firstComeFirstServe(vector<Process>& processes, ofstream& fcfs) {
    // Sort the processes by arrival time
    sortByArrivalTime(processes);
    // Start time is the arrival time of the first process
    int startTime = processes[0].arrivalTime;
    // Loop through the processes
    for(auto& p : processes) {
        // Calculate the wait time
        int waitTime = startTime - p.arrivalTime;
        // Calculate the turnaround time
        int turnaroundTime = waitTime + p.serviceTime;
        // Write the results to the file
        for(int i = 0; i < ((turnaroundTime-waitTime)/10); i++) {
            fcfs << p.pid << "\n";
        }
        // Update the start time
        startTime += p.serviceTime;
    }
}
void roundRobin(vector<Process>& processes, int quantumTime, ofstream& rr) {
    // Sort the processes by arrival time
    sortByArrivalTime(processes);
    // FIFO Queue
    queue<Process> q;
    // Keep track of the current time
    int currentTime = 0;
    int processIndex = 0;
    while(!q.empty() || processIndex < processes.size()) {
    while(processIndex < processes.size() && processes[processIndex].arrivalTime <= currentTime) {
        q.push(processes[processIndex]);
        processIndex++;
    }

    if (!q.empty()) {
        Process currentProcess = q.front();
        q.pop();

        int timeSlice = min(quantumTime, currentProcess.serviceTime);
        currentTime += timeSlice;
        currentProcess.serviceTime -= timeSlice;

        while(processIndex < processes.size() && processes[processIndex].arrivalTime <= currentTime) {
            q.push(processes[processIndex]);
            processIndex++;
        }

        if(currentProcess.serviceTime > 0) {
            q.push(currentProcess);
        }

        for(int i = 0; i < timeSlice/10; i++) {
            rr << currentProcess.pid << "\n";
        }
    } else {
        currentTime = processes[processIndex].arrivalTime;
    }
    }
}
void shortestProcessNext(vector<Process>& processes, ofstream& spn) {
    // Sort the processes by arrival time
    sortByArrivalTime(processes);

    // to me it makes sense to use a priority queue here
    // attempted with no structure at all, but 
    priority_queue<Process, vector<Process>, CompareServiceTime> pq;

    int currentTime = 0;
    int processIndex = 0;

    while (!pq.empty() || processIndex < processes.size()) {
        // push them onto pqueue
        while (processIndex < processes.size() && processes[processIndex].arrivalTime <= currentTime) {
            pq.push(processes[processIndex]);
            processIndex++;
        }

        // If the priority queue is not empty, follow spin logic
        if (!pq.empty()) {
            //Grab the top one, as its always first.
            Process currentProcess = pq.top();
            pq.pop();
            currentTime += currentProcess.serviceTime;
            for(int i = 0; i < currentProcess.serviceTime/10; i++) {
                spn << currentProcess.pid << "\n";
            }
        } else {
            // If the priority queue is empty, increment the current time to the arrival time of the next process
            currentTime = processes[processIndex].arrivalTime;
        }
    }
}
void highestResponseRatioNext(vector<Process>& processes, ofstream& hrrn) {
    // as usual
    sortByArrivalTime(processes);

    int currentTime = 0;
    vector<Process> arrivedProcesses;

    while (!processes.empty() || !arrivedProcesses.empty()) {
        // add the new arrivals to the vector, which is just a queue in disguise
        auto it = processes.begin();
        while (it != processes.end()) {
            if (it->arrivalTime <= currentTime) {
                arrivedProcesses.push_back(*it);
                it = processes.erase(it);
            } else {
                ++it;
            }
        }

        // if nothing is there, (arrival time =/= 0)
        if (arrivedProcesses.empty()) {
            currentTime++;
            continue;
        }

        // get the response ratios, add them to queue
        vector<double> responseRatios;
        for (const Process& process : arrivedProcesses) {
            int waitingTime = currentTime - process.arrivalTime;
            double responseRatio = (waitingTime + process.serviceTime) / (double)process.serviceTime;
            responseRatios.push_back(responseRatio);
        }

        // get the max
        int maxIndex = distance(responseRatios.begin(), max_element(responseRatios.begin(), responseRatios.end()));
        Process selectedProcess = arrivedProcesses[maxIndex];

        // write it tot he file
        currentTime += selectedProcess.serviceTime;
        // write in time increments of 10
        for(int i = 0; i < selectedProcess.serviceTime/10; i++) {
            hrrn << selectedProcess.pid << "\n";
        }

        // remove the max
        arrivedProcesses.erase(arrivedProcesses.begin() + maxIndex);
    }
}
void shortestRemainingTime(vector<Process>& processes, ofstream& srt) {
    // Do the usual
    sortByArrivalTime(processes);
    // set up our priority queue, based on burst time
    priority_queue<Process, vector<Process>, CompareServiceTime> pq;
    int currentTime = 0;
    // Loop through the processes
    while(!pq.empty() || !processes.empty()) {
        // Add the new arrivals to the priority queue
        auto it = processes.begin();
        while(it != processes.end()) {
            if(it->arrivalTime <= currentTime) {
                pq.push(*it);
                it = processes.erase(it);
            } else {
                ++it;
            }
        }
        // If the priority queue is empty, increment the current time to the arrival time of the next process
        if(pq.empty()) {
            currentTime = processes[0].arrivalTime;
            continue;
        }
        // Get the process with the shortest remaining time
        // Do SRT logic
        Process currentProcess = pq.top();
        pq.pop();
        currentTime += 10;
        currentProcess.serviceTime -= 10;
        // Write the process to the file
        srt << currentProcess.pid << "\n";
        // If the process still has service time left when a new one arrives, push it back onto the priority queue
        if(currentProcess.serviceTime > 0) {
            pq.push(currentProcess);
        }
    }
}
    

// Main function
int main(int argc, char *argv[]) {
    // Do; file error handling
    if(argc < 2) {
        cerr << "No file name provided. Please provide an input file as an argument.\n";
        return 1;
    }

    // Open the file
    ifstream inputFile(argv[1]);

    // Make sure the file exists
    if(!inputFile.is_open()) {
        cerr << "Could not open file " << argv[1] << ".\n";
        return 1;
    }
    // open the necessary output files
    ofstream fcfs("fcfs.out");
    ofstream rr10("rr_10.out");
    ofstream rr40("rr_40.out");
    ofstream srt("srt.out");
    ofstream spn("spn.out");
    ofstream hrrn("hrrn.out");
    ofstream feedback("feedback.out");
    
    // Process the file
    vector<Process> processes;
    string line;
    while(getline(inputFile, line)) {
        istringstream iss(line);
        Process p;
        // This is a debug line to help me with custom input files
        if(!(iss >> p.pid >> p.arrivalTime >> p.serviceTime)) {
            cerr << "Invalid data format.\n";
            return 1;
        }
        // Add the processes to our set
        processes.push_back(p);
    }
    // Call the scheduling algorithms
    //Done
    firstComeFirstServe(processes, fcfs);
    vector<Process> processesCopy1 = processes;
    //Done
    roundRobin(processesCopy1, 10, rr10);
    //Done
    vector<Process> processesCopy2 = processes;
    roundRobin(processesCopy2, 40, rr40); 
    //Done
    vector<Process> processesCopy3 = processes;
    shortestProcessNext(processesCopy3, spn);
    //Done
    vector<Process> processesCopy4 = processes;
    highestResponseRatioNext(processesCopy4, hrrn);
    //Done
    vector<Process> processesCopy5 = processes;
    shortestRemainingTime(processesCopy5, srt);
    inputFile.close();


    return 0;
}