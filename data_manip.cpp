#include "headers/individual.h"

// Empty constructor
Data::Data() : num_machines(0), total_operations(0) {}

bool Data::loadInstance(const vvf& time, const vvf& energy, const vvi& jobs){
    if(time.empty() || energy.empty()) return false;
    
    task_energy = energy;
    task_time = time;
    
    num_machines = time[0].size(); // Could be energy[0].size(), they have the same size
    total_jobs = jobs.size();
    total_operations = 0;
    
    for(const auto& work : jobs) total_operations += work.size();
    
    return true;
}


int Data::getNumMachines() const{
    return num_machines;   
}

int Data::getNumTasks() const{
    return total_operations;
}

int Data::getNumJobs() const{
    return total_jobs;
}

// Operation and Machine in the table
float Data::getTime(int task, int machine) const{ 
    return task_time[task][machine];
}

// Operation and Machine in the table
float Data::getEnergy(int task, int machine) const{
    return task_energy[task][machine];
}