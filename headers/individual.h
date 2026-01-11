#ifndef INDIVIDUAL_H
#define INDIVIDUAL_H

#pragma once

#include <vector>

using vvi = std::vector<std::vector<int>>;
using vvf = std::vector<std::vector<float>>;

enum class PolicyType{
    FIFO,
    LTP,
    STP,
    RR_FIFO,
    RR_LTP,
    RR_ECA
};

template <typename T>
struct BasePolicies{
    T fifo    = T{}; // 0 for int and 0.0f for float
    T ltp     = T{};
    T stp     = T{};
    T rr_fifo = T{};
    T rr_ltp  = T{};
    T rr_eca  = T{};

    T getValue(PolicyType policy) const{
        switch(policy){
            case PolicyType::FIFO:    return fifo;
            case PolicyType::LTP:     return ltp;
            case PolicyType::STP:     return stp;
            case PolicyType::RR_FIFO: return rr_fifo;
            case PolicyType::RR_LTP:  return rr_ltp;
            case PolicyType::RR_ECA:  return rr_eca;
            default:                  return T{};
        }
    }

    void setValue(PolicyType policy, T value){
        switch(policy){
            case PolicyType::FIFO:    fifo = value; break;
            case PolicyType::LTP:     ltp  = value; break;
            case PolicyType::STP:     stp  = value; break;
            case PolicyType::RR_FIFO: rr_fifo = value; break;
            case PolicyType::RR_LTP:  rr_ltp  = value; break;
            case PolicyType::RR_ECA:  rr_eca  = value; break;
        }
    }
};

using Policies     = BasePolicies<float>;
using RankPolicies = BasePolicies<int>;

struct Chromosome{
    std::vector<int> fifo;
    std::vector<int> ltp;
    std::vector<int> stp;
    std::vector<int> rr_fifo;
    std::vector<int> rr_ltp;
    std::vector<int> rr_eca;

    std::vector<int>& getValue(PolicyType policy){
        switch(policy){
            case PolicyType::FIFO:    return fifo;
            case PolicyType::LTP:     return ltp;
            case PolicyType::STP:     return stp;
            case PolicyType::RR_FIFO: return rr_fifo;
            case PolicyType::RR_LTP:  return rr_ltp;
            case PolicyType::RR_ECA:  return rr_eca;
            default:                  return fifo;
        }
    }

    void setValue(PolicyType policy, std::vector<int>& value){
        switch(policy){
            case PolicyType::FIFO:    fifo = value; break;
            case PolicyType::LTP:     ltp  = value; break;
            case PolicyType::STP:     stp  = value; break;
            case PolicyType::RR_FIFO: rr_fifo = value; break;
            case PolicyType::RR_LTP:  rr_ltp  = value; break;
            case PolicyType::RR_ECA:  rr_eca  = value; break;
        }
    }
};

struct Individual{
    Chromosome chromosome;
    
    Policies time_fitness;
    Policies energy_fitness;

    Policies crowding_distance;
    RankPolicies rank; // Dominance level
};

struct IndividualComparator{
    // What policy will it compare?
    PolicyType policy;

    IndividualComparator(PolicyType p) : policy(p){}

    // std::sort will use this
    bool operator()(const Individual& a, const Individual& b) const{
        // Compare ranks
        int rank_a = a.rank.getValue(policy);
        int rank_b = b.rank.getValue(policy);

        if(rank_a < rank_b){
            return true;
        }
        if(rank_a > rank_b){
            return false;
        }

        // If the rank is the same, compare by CD
        float cd_a = a.crowding_distance.getValue(policy);
        float cd_b = b.crowding_distance.getValue(policy);

        // Descendending
        return cd_a > cd_b;
    }
};

class Data{
private:
    int num_machines; // max_work
    int total_operations; 
    int total_jobs;

    vvf task_time;
    vvf task_energy;

public:
    Data();

    bool loadInstance(const vvf& time, const vvf& energy, const vvi& jobs);

    int getNumMachines() const;
    int getNumTasks() const;
    int getNumJobs() const;
    
    float getTime(int task, int machine) const;
    float getEnergy(int task, int machine) const;
};

#endif // INDIVIDUAL_H