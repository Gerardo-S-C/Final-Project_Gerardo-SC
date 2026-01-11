#include "headers/read_test.h"

#include <iostream>
#include <sstream>
#include <string>


std::vector<std::vector<float>> loadTime(std::ifstream& f){
    using namespace std;
    // Size of operations and machines available
    string constraint;
    getline(f, constraint);
    
    int a, b = 0;
    // Validate
    if(constraint.size() >= 3){ // two int chars and one space char
        //! Only two numbers (if it's 2D)
        int pos = constraint.find(' ');
    
        a = stoi(constraint.substr(0));
        b = stoi(constraint.substr(pos));
    }else{
        cerr << "Unreadable input for time data\nValidate the file\n\n";
        return {{0.0f}};
    }
    
    vector<vector<float>> time(a, vector<float>(b));

    for(size_t i = 0; i < a; ++i){
        getline(f, constraint);
        stringstream ss(constraint);
        vector<float> value;
        string num;
        for(size_t j = 0; j < b; ++j){
            getline(ss, num, ' ');
            float n = stof(num);
            value.push_back(n);
        }
        time.at(i) = value;
    }
    
    return time;
}

std::vector<std::vector<float>> loadEnergy(std::ifstream& f){
    using namespace std;
    // Size of operations and machines available
    string constraint;
    getline(f, constraint);
    
    int a, b = 0;
    // Validate
    if(constraint.size() >= 3){ // two int chars and one space char
        // Only two numbers (if it's 2D)
        int pos = constraint.find(' ');
    
        a = stoi(constraint.substr(0));
        b = stoi(constraint.substr(pos));
    }else{
        cerr << "Unreadable input for energy data\nValidate the file\n\n";
        return {{0.0f}};
    }
    
    vector<vector<float>> energy(a, vector<float>(b));

    for(size_t i = 0; i < a; ++i){
        getline(f, constraint);
        stringstream ss(constraint);
        vector<float> value;
        string num;
        for(size_t j = 0; j < b; ++j){
            getline(ss, num, ' ');
            float n = stof(num);
            value.push_back(n);
        }
        energy.at(i) = value;
    }
    
    return energy;
}

std::vector<std::vector<int>> loadWork(std::ifstream& f){
    using namespace std;
    // Operations for each machine
    string constraint;

    getline(f, constraint);
    int a = 0;
    
    if(constraint.find(' ') != string::npos){
        cerr << "There's more than 1 value, check the data\n";
        return {};
    }
    a = stoi(constraint);
    vector<vector<int>> work(a);
    
    for(size_t i = 0; i < a; ++i){
        getline(f, constraint);
        stringstream ss(constraint);
        vector<int> workload;
        string num;

        while(getline(ss, num, ' ')){
            int n = stoi(num);
            workload.push_back(n);
        }

        work.at(i) = workload;
    }

    return work;
}