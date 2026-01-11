#ifndef POLICIES_H
#define POLICIES_H

#pragma once

#include <vector>

using vvi = std::vector<std::vector<int>>;
using vvf = std::vector<std::vector<float>>;

struct JobStats{
    float min_time;
    float max_time;
    float avg_time;
};

struct OperationsID{
    int job_idx;
    int op_idx; 
};
using vec_op = std::vector<OperationsID>;

vec_op FIFO   (const int& tasks, const vvi& jobs);
vec_op LTP    (const int& tasks, const vvi& jobs, const vvf& time);
vec_op STP    (const int& tasks, const vvi& jobs, const vvf& time);
vec_op RR_FIFO(const int& tasks, const vvi& jobs);
vec_op RR_LTP (const int& tasks, const vvi& jobs, const vvf& time);
vec_op RR_ECA (const int& tasks, const vvi& jobs, const vvf& energy);

#endif // POLICIES_H