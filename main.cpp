#include "headers/read_test.h"
#include "headers/policies.h"
#include "headers/individual.h"
#include "headers/nsgaII.h"

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

int main(){
    using namespace std;

    // 3 test cases provided
    vector<string> test_cases = {"Eg1", "Eg2", "Eg3"};

    for(const string& instance_name : test_cases){
        string current_file_path = "test/" + instance_name + ".txt";
        
        cout << "\n========================================\n";
        cout << "PROCESSING INSTANCE: " << instance_name << "\n";
        cout << "File path: " << current_file_path << "\n";
        cout << "========================================\n";

        ifstream file(current_file_path);

        if(!file.is_open()){
            cerr << "Test case file not found: " << current_file_path << endl;
            continue; // Skip to the next test case
        }

        // Load data
        vector<vector<float>> time = loadTime(file);
        vector<vector<float>> energy = loadEnergy(file);

        if(time.size() != energy.size() || time[0].size() != energy[0].size()){
            cerr << "Time and energy constraints don't match in size, check the data in " << instance_name << "\n";
            file.close();
            continue;
        }

        vector<vector<int>> jobs = loadWork(file);
        
        if(jobs.empty()){
            cerr << "There's no workload to work with in " << instance_name << "\n";
            file.close();
            continue;
        }
        file.close();
        
        cout << "Data read from file succesfully\n";

        Data data;
        if(!data.loadInstance(time, energy, jobs)){
            cerr << "Could not load data correctly for some reason...\n";
            continue;
        }else cout << "Instance correctly loaded\n";

        const int tasks_tbd = data.getNumTasks();

        // Generate policies orderings
        auto fifo = FIFO(tasks_tbd, jobs);
        auto ltp  = LTP (tasks_tbd, jobs, time);
        auto stp  = STP (tasks_tbd, jobs, time);
        
        // Round Robin
        auto rr_fifo = RR_FIFO(tasks_tbd, jobs);
        auto rr_ltp  = RR_LTP (tasks_tbd, jobs, time);
        auto rr_eca  = RR_ECA (tasks_tbd, jobs, energy);
        
        //* NSGAII main loop preparation
        unordered_map<PolicyType, vec_op> policies_map;
        
        policies_map[PolicyType::FIFO]    = fifo;
        policies_map[PolicyType::LTP]     = ltp;
        policies_map[PolicyType::STP]     = stp;
        policies_map[PolicyType::RR_FIFO] = rr_fifo;
        policies_map[PolicyType::RR_LTP]  = rr_ltp;
        policies_map[PolicyType::RR_ECA]  = rr_eca;

        mainLoop(data, policies_map, instance_name);
    }
    
    cout << "\nALL INSTANCES COMPLETED.\n";
    return 0;
}