#include "headers/policies.h"

#include <algorithm>
#include <cfloat>
#include <numeric>
#include <iostream>
size_t max_cols(const vvi& jobs){
    size_t cols = 0;
    for(const auto& row : jobs) cols = std::max(cols, row.size());
    return cols;
}

// @brief It returns min, max and avg as stats for the policies
std::vector<JobStats> stats(const vvi& jobs, const vvf& operations){
    std::vector<JobStats> full_stats(operations.size());
    std::vector<JobStats> op_stats(jobs.size());

    for(int i = 0; i < operations.size(); ++i){
        auto [min, max] = std::minmax_element(operations[i].begin(), operations[i].end());
        float avg = std::accumulate(operations[i].begin(), operations[i].end(), 0.0f) / operations[i].size();

        full_stats[i].min_time = *min;
        full_stats[i].max_time = *max;
        full_stats[i].avg_time = avg; // roundf(avg * 10.0f) / 10.0f; //cfloat
    }

    for(int i = 0; i < jobs.size(); ++i){
        float min = 0.0f;
        float max = 0.0f;
        float avg = 0.0f;
        for(int j = 0; j < jobs[i].size(); ++j){
            min += full_stats[jobs[i][j] - 1].min_time;
            max += full_stats[jobs[i][j] - 1].max_time;
            avg += full_stats[jobs[i][j] - 1].avg_time;
        }
        op_stats[i].min_time = min;
        op_stats[i].max_time = max;
        op_stats[i].avg_time = avg;
    }

    return op_stats;
}

vec_op FIFO(const int& tasks, const vvi& jobs){
    int idx = 0;
    vec_op fifo(tasks);
    
    for(int i = 0; i < jobs.size(); ++i){
        for(int j = 0; j < jobs[i].size(); ++j){
            fifo[idx++] = {i, jobs[i][j] - 1};
        }
    }

    return fifo;
}

vec_op LTP(const int& tasks, const vvi& jobs, const vvf& operations){
    int idx = 0;
    vec_op ltp(tasks);
    std::vector<JobStats> time = stats(jobs, operations);
    std::vector<int> index(jobs.size());
    std::iota(index.begin(), index.end(), 0);

    std::sort(index.begin(), index.end(), [&time](size_t i, size_t j){
        return time[i].max_time > time[j].max_time;
    });

    for(int i : index){
        for(int j = 0; j < jobs[i].size(); ++j){
            ltp[idx++] = {i, jobs[i][j] - 1}; 
        }
    }

    return ltp;
}

vec_op STP(const int& tasks, const vvi& jobs, const vvf& operations){
    int idx = 0;
    vec_op stp(tasks);
    std::vector<JobStats> time = stats(jobs, operations);
    std::vector<int> index(jobs.size());
    std::iota(index.begin(), index.end(), 0);

    std::sort(index.begin(), index.end(), [&time](size_t i, size_t j){
        return time[i].min_time < time[j].min_time;
    });

    for(int i : index){
        for(int j = 0; j < jobs[i].size(); ++j){
            stp[idx++] = {i, jobs[i][j] - 1}; 
        }
    }
    
    return stp;
}

vec_op RR_FIFO(const int& tasks, const vvi& jobs){
    int idx = 0;
    size_t cols = max_cols(jobs);
    vec_op rr_fifo(tasks);
    
    for(int i = 0; i < cols; ++i){
        for(int j = 0; j < jobs.size(); ++j){
            if(i < jobs[j].size()){
                rr_fifo[idx++] = {j, jobs[j][i] - 1};
            }
        }
    }

    return rr_fifo;
}

vec_op RR_LTP(const int& tasks, const vvi& jobs, const vvf& operations){
    int idx = 0;
    vec_op rr_ltp(tasks);
    size_t cols = max_cols(jobs);
    std::vector<JobStats> time = stats(jobs, operations);
    std::vector<int> index(jobs.size());
    std::iota(index.begin(), index.end(), 0);

    std::sort(index.begin(), index.end(), [&time](size_t i, size_t j){
        return time[i].max_time > time[j].max_time;
    });

    for(size_t j = 0; j < cols; ++j){
        for(int i : index){
            if(j < jobs[i].size()){
                rr_ltp[idx++] = {i, jobs[i][j] - 1};
            }
        }
    }

    return rr_ltp;
}

vec_op RR_ECA(const int& tasks, const vvi& jobs, const vvf& operations){
    int idx = 0;
    vec_op rr_eca(tasks);
    size_t cols = max_cols(jobs);
    std::vector<JobStats> energy = stats(jobs, operations);
    std::vector<int> index(jobs.size());
    std::iota(index.begin(), index.end(), 0);

    std::sort(index.begin(), index.end(), [&energy](size_t i, size_t j){
        return energy[i].avg_time < energy[j].avg_time;
    });

    for(size_t j = 0; j < cols; ++j){
        for(int i : index){
            if(j < jobs[i].size()){
                rr_eca[idx++] = {i, jobs[i][j] - 1};
            }
        }
    }

    return rr_eca;
}