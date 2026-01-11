#ifndef NSGAII_H
#define NSGAII_H

#include "individual.h"
#include "policies.h"

#include <unordered_map>
#include <string>

#pragma once

// Structure of the time in every work load by machine to make gantt diagrams
struct Gantt{
    int job;
    int operation;
    float initial_time;
    float end_time;
    //float energy;
};

void mainLoop(const Data& data, const std::unordered_map<PolicyType, vec_op>& policies_order, const std::string& instance_name);

#endif // NSGAII_H