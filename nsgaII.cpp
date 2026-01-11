#include "headers/nsgaII.h"

#include <fstream>
#include <algorithm>
#include <utility>
#include <numeric>
#include <random>
#include <filesystem>
#include <iostream>

#define POPULATION_SIZE 10
#define GENERATION_SIZE 100

struct AdaptiveParams {
    int crossover_prob;
    int mutation_prob;
};

static const std::vector<PolicyType> policy_index_map = {
    PolicyType::FIFO,
    PolicyType::LTP,
    PolicyType::STP,
    PolicyType::RR_FIFO,
    PolicyType::RR_LTP,
    PolicyType::RR_ECA
};

std::string policyToString(PolicyType policy) {
    switch (policy) {
        case PolicyType::FIFO:    return "FIFO";
        case PolicyType::LTP:     return "LTP";
        case PolicyType::STP:     return "STP";
        case PolicyType::RR_FIFO: return "RR_FIFO";
        case PolicyType::RR_LTP:  return "RR_LTP";
        case PolicyType::RR_ECA:  return "RR_ECA";
        default:                  return "Unknown";
    }
}

void ensureDirectoryExists(const std::string& path){
    namespace fs = std::filesystem;
    if (!fs::exists(path)) {
        fs::create_directories(path);
    }
}

void saveGanttToFile(const std::string& file_path, const std::vector<std::vector<Gantt>>& gantt_chart){
    std::ofstream output_file(file_path);
    if(!output_file.is_open()){
        // Try to create the directory if it doesn't exist
        size_t pos = file_path.find_last_of("/\\");
        if(pos != std::string::npos) ensureDirectoryExists(file_path.substr(0, pos));
        
        output_file.open(file_path);
        if(!output_file.is_open()){
             std::cerr << "Error: No se pudo abrir el archivo de Gantt: " << file_path << std::endl;
             return;
        }
    }
    
    // Header
    output_file << "Machine_ID,Job_ID,Operation_ID,Start_Time,End_Time\n";
    
    for(size_t machine_id = 0; machine_id < gantt_chart.size(); ++machine_id){
        for (const auto& task : gantt_chart[machine_id]) {
            output_file << machine_id + 1 << ","
            << task.job + 1 << ","
            << task.operation + 1 << ","
            << task.initial_time << ","
            << task.end_time << "\n";
        }
    }
    
    output_file.close();
    // std::cout << "Gantt saved in '" << file_path << "'." << std::endl;
}

// Generate a random chromosome for the first(s) generation(s)
Individual generateChromosome(const Data& data, std::mt19937& gen){
    Individual individual;
    int total_ops    = data.getNumTasks();
    int num_machines = data.getNumMachines();

    std::uniform_int_distribution<> uid(0, num_machines - 1);

    for(const auto& pol : policy_index_map){
        PolicyType policy = pol;
        
        std::vector<int> chromo_vec;
        chromo_vec.resize(total_ops);
        for (int i = 0; i < total_ops; ++i) {
            chromo_vec[i] = uid(gen);
        }
        individual.chromosome.setValue(policy, chromo_vec);
    }

    return individual;
}

std::pair<float, std::vector<std::vector<Gantt>>> totalTime(const Data& data, Individual& ind, PolicyType& policy, 
const std::unordered_map<PolicyType, vec_op>& policies_order){

    int machines = data.getNumMachines();
    int num_op   = data.getNumTasks();
    int num_job  = data.getNumJobs();

    std::vector<float> job_end_time(num_job, 0.0f);
    std::vector<float> machine_end_time(machines, 0.0f);
    
    std::vector<std::vector<Gantt>> total_work(machines);

    const std::vector<int> chromosome_to_eval = ind.chromosome.getValue(policy);
    
    // Until every operation from every task is done
    for(size_t priority = 0; priority < num_op; ++priority){
        int machine     = chromosome_to_eval.at(priority); 
        int current_job = policies_order.at(policy).at(priority).job_idx;
        int current_op  = policies_order.at(policy).at(priority).op_idx;
        
        float time_to_add          = data.getTime(current_op, machine);
        float current_job_time     = job_end_time[current_job];
        float current_machine_time = machine_end_time[machine];

        float best_time = std::max(current_job_time, current_machine_time);
        float prev_time = best_time;

        best_time += time_to_add;
        job_end_time[current_job] = best_time;
        machine_end_time[machine] = best_time;

        /*Gantt Diagram space*/
        Gantt entry;
        entry.job = current_job;
        entry.operation = current_op;
        entry.initial_time = prev_time;
        entry.end_time = best_time;
        total_work[machine].push_back(entry);
        /*-------------------*/
    }

    float makespan = 0.0f;
    makespan = *std::max_element(machine_end_time.begin(), machine_end_time.end());

    return {makespan, total_work};
}

float totalEnergy(const Data& data, Individual& ind, PolicyType& policy, const std::unordered_map<PolicyType, vec_op>& policies_order){
    float tot_energy = 0.0f;
    int operation, machine;
    int tasks = data.getNumTasks();

    const std::vector<int> chromosome_to_eval = ind.chromosome.getValue(policy);

    for(int i = 0; i < tasks; ++i){
        operation = policies_order.at(policy).at(i).op_idx;
        machine   = chromosome_to_eval.at(i);
        
        tot_energy += data.getEnergy(operation, machine);
    }

    return tot_energy;
}

//* Genetic algorithm stuff *//

// Uniform polyploid cross
std::pair<Individual, Individual> crossover(Individual& parent1, Individual& parent2, 
const std::unordered_map<PolicyType, vec_op>& policies_order, std::mt19937& gen, int current_crossover_prob){
    // Probability param
    std::uniform_int_distribution<> uid(0, 99);

    // Offsprings
    Individual child1, child2;
    child1.chromosome = parent1.chromosome;
    child2.chromosome = parent2.chromosome;

    int size = parent1.chromosome.getValue(PolicyType::FIFO).size();
    
    std::vector<bool> swap_idx(size, false); // Indices to be swapped
    for(size_t i = 0; i < size; ++i){
        if(uid(gen) < current_crossover_prob){
            swap_idx[i] = true;
        }
    }

    for(const auto& pol_pair : policies_order){
        PolicyType policy = pol_pair.first;

        std::vector<int>& chrom1 = child1.chromosome.getValue(policy);
        std::vector<int>& chrom2 = child2.chromosome.getValue(policy);

        // Swap
        for(size_t i = 0; i < size; ++i){
            if(swap_idx[i]){
                std::swap(chrom1[i], chrom2[i]);
            }
        }
    }
    
    return {child1, child2};
}

// Different mutations
void interChrome(Individual& individual, std::mt19937& gen){
    std::uniform_int_distribution<> uid(0, policy_index_map.size() - 1); // To select two of the chromosomes
    int l = uid(gen);
    int r = uid(gen);
    // We make sure they are different
    while(l == r) r = uid(gen);
    
    // Policies to be swapped
    PolicyType pol1 = policy_index_map[l];
    PolicyType pol2 = policy_index_map[r];

    std::vector<int>& chrom1 = individual.chromosome.getValue(pol1);    
    std::vector<int>& chrom2 = individual.chromosome.getValue(pol2);    

    std::swap(chrom1, chrom2);
}

void equitativeExchange(Individual& individual, std::mt19937& gen){
    size_t pair_size = individual.chromosome.getValue(PolicyType::FIFO).size() - 1;
    if(pair_size < 1) return; 

    std::uniform_int_distribution<> uid_pair(0, pair_size / 2);
    std::uniform_int_distribution<> uid(0, pair_size);

    // Each policy might mutate differently
    for(const auto& policy : policy_index_map){
        // Selection of the number of pairs and it's idx
        int total_pairs = uid_pair(gen);
        std::vector<std::pair<int, int>> chrom_pair;
        
        // Safety check to avoid infinite loop if size is too small
        int attempts = 0;
        while(chrom_pair.size() != total_pairs && attempts < 100){
            int l = uid(gen);
            int r = uid(gen);
            while(l == r) r = uid(gen);
            chrom_pair.push_back({l, r});
            attempts++;
        }
        
        std::vector<int>& chrom = individual.chromosome.getValue(policy);

        for(const auto& pr : chrom_pair){
            std::swap(chrom[pr.first], chrom[pr.second]);
        }

    }
}

void circular(Individual& individual, std::mt19937& gen){
    size_t chromo_size = individual.chromosome.getValue(PolicyType::FIFO).size();
    if(chromo_size < 2) return;

    std::uniform_int_distribution<> uid(0, chromo_size - 1);

    for(const auto& policy : policy_index_map){
        std::vector<int>& chromo = individual.chromosome.getValue(policy);

        // 1. Select Segment
        int l = uid(gen);
        int r = uid(gen);
        int start = std::min(l, r);
        int end   = std::max(l, r);
        
        // Copy segment
        std::vector<int> segment;
        for(int i=start; i<=end; ++i) segment.push_back(chromo[i]);

        // 2. Erase segment from original (Cut)
        chromo.erase(chromo.begin() + start, chromo.begin() + end + 1);

        // 3. Insert in new position (Paste)
        // New size is smaller, so we gen a new index
        std::uniform_int_distribution<> uid_ins(0, chromo.size()); 
        int ins_point = uid_ins(gen);
        
        chromo.insert(chromo.begin() + ins_point, segment.begin(), segment.end());
    }
}

//* Main NSGAII Algorithm *//
std::vector<std::vector<int>> fastNonDominatedSort(std::vector<Individual>& population, const PolicyType& policy){
    // Domination
    std::vector<std::vector<int>> dominated_solutions(population.size());
    std::vector<int> domination_count(population.size(), 0);

    // First: Calculate which solutions dominate others for ONE policy
    for(size_t i = 0; i < population.size(); ++i){
        for (size_t j = i + 1; j < population.size(); ++j){
            // Values from current policy
            float time_i   = population[i].time_fitness.getValue(policy);
            float energy_i = population[i].energy_fitness.getValue(policy);
            float time_j   = population[j].time_fitness.getValue(policy);
            float energy_j = population[j].energy_fitness.getValue(policy);

            bool i_dominates_j = (time_i <= time_j && energy_i <= energy_j) &&
                                 (time_i < time_j || energy_i < energy_j);
            bool j_dominates_i = (time_j <= time_i && energy_j <= energy_i) &&
                                 (time_j < time_i || energy_j < energy_i);

            if(i_dominates_j){
                dominated_solutions[i].push_back(j);
                domination_count[j]++;
            }else if(j_dominates_i){
                dominated_solutions[j].push_back(i);
                domination_count[i]++;
            }
        }
    }
    // Then, identify the first front
    std::vector<std::vector<int>> fronts_indices;
    std::vector<int> current_front_indices;
    for(size_t i = 0; i < population.size(); ++i){
        if(domination_count[i] == 0){
            population[i].rank.setValue(policy, 1);
            current_front_indices.push_back(i);
        }
    }
    fronts_indices.push_back(current_front_indices);

    // After that, build the next fronts
    int rank = 1;
    while(!fronts_indices[rank - 1].empty()){
        std::vector<int> next_front_indices;
        // For each individual 'p' in the actual front
        for(int p_idx : fronts_indices[rank - 1]){
            // looks all the solutions 'q' that it dominates
            for(int q_idx : dominated_solutions[p_idx]){
                // and decrease it's counter of domination
                domination_count[q_idx]--;
                // If the counter reaches 0, 'q' belongs in the next front
                if(domination_count[q_idx] == 0){
                    population[q_idx].rank.setValue(policy, rank + 1);
                    next_front_indices.push_back(q_idx);
                }
            }
        }
        
        if(next_front_indices.empty()) break;
        
        fronts_indices.push_back(next_front_indices);
        rank++;
    }
    
    return fronts_indices;
}

void calculateCrowdingDistance(std::vector<Individual>& front, const PolicyType& policy){
    if (front.empty()) return;
    int size = front.size();
    for(auto& ind : front){
        ind.crowding_distance.setValue(policy, 0);
    }

    // Obj 1: Time
    std::sort(front.begin(), front.end(), [policy](const Individual& a, const Individual& b){
        return a.time_fitness.getValue(policy) < b.time_fitness.getValue(policy);
    });
    front[0].crowding_distance.setValue(policy, 1e9); // Infinity
    front.back().crowding_distance.setValue(policy, 1e9);
    
    float min_time = front[0].time_fitness.getValue(policy);
    float max_time = front.back().time_fitness.getValue(policy);
    float range_time = max_time - min_time;
    
    if(range_time > 0){
        for(int i = 1; i < size - 1; ++i){
            float current_cd = front[i].crowding_distance.getValue(policy);
            current_cd += (front[i + 1].time_fitness.getValue(policy) - front[i - 1].time_fitness.getValue(policy)) / range_time;
            front[i].crowding_distance.setValue(policy, current_cd);
        }
    }

    // Obj 2: Energy
    std::sort(front.begin(), front.end(), [policy](const Individual& a, const Individual& b){
        return a.energy_fitness.getValue(policy) < b.energy_fitness.getValue(policy);
    });
    front[0].crowding_distance.setValue(policy, 1e9);
    front.back().crowding_distance.setValue(policy, 1e9);
    
    float min_energy = front[0].energy_fitness.getValue(policy);
    float max_energy = front.back().energy_fitness.getValue(policy);
    float range_energy = max_energy - min_energy;

    if(range_energy > 0){
        for(int i = 1; i < size - 1; ++i){
            float current_cd = front[i].crowding_distance.getValue(policy);
            current_cd += (front[i + 1].energy_fitness.getValue(policy) - front[i - 1].energy_fitness.getValue(policy)) / range_energy;
            front[i].crowding_distance.setValue(policy, current_cd);
        }
    }
}

// Tournament selection
Individual tournamentSelection(std::vector<Individual>& population, std::mt19937& gen){
    std::uniform_int_distribution<> uid(0, population.size() - 1);
    int idx1 = uid(gen);
    int idx2 = uid(gen);
    while(idx1 == idx2) idx2 = uid(gen);

    Individual& parent1 = population[idx1];
    Individual& parent2 = population[idx2];

    Individual super_individual;

    for(const PolicyType& policy : policy_index_map){
        // Exclusive comparator for the Individuals
        IndividualComparator comparator(policy);

        if(comparator(parent1, parent2)){
            // Parent 1 is better in the current policy
            super_individual.chromosome.setValue(policy, parent1.chromosome.getValue(policy));
            super_individual.time_fitness.setValue(policy, parent1.time_fitness.getValue(policy));
            super_individual.energy_fitness.setValue(policy, parent1.energy_fitness.getValue(policy));
            super_individual.rank.setValue(policy, parent1.rank.getValue(policy));
            super_individual.crowding_distance.setValue(policy, parent1.crowding_distance.getValue(policy));
        }else{
            // Parent 2 is better in the current policy... or they're exactly the same
            super_individual.chromosome.setValue(policy, parent2.chromosome.getValue(policy));
            super_individual.time_fitness.setValue(policy, parent2.time_fitness.getValue(policy));
            super_individual.energy_fitness.setValue(policy, parent2.energy_fitness.getValue(policy));
            super_individual.rank.setValue(policy, parent2.rank.getValue(policy));
            super_individual.crowding_distance.setValue(policy, parent2.crowding_distance.getValue(policy));
        }
    }

    return super_individual;
}

void mainLoop(const Data& data, const std::unordered_map<PolicyType, vec_op>& policies_order, const std::string& instance_name){
    // 30 Semillas requeridas
    const int seeds[] = {0, 1, 2, 3, 5, 7, 11, 13, 17, 19, 
                         23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
                         67, 71, 73, 79, 83, 89, 97, 101, 103, 107};

    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<int> prob_gen(0, 100);
    
    // Main GA loop params
    const int population_size = POPULATION_SIZE;
    const int generation_size = GENERATION_SIZE;
    
    std::uniform_int_distribution<> random_parent(0, population_size - 1);
    
    // Output base path
    std::string base_path = "results/" + instance_name;
    ensureDirectoryExists(base_path);
    for(const auto& pol : policy_index_map) ensureDirectoryExists(base_path + "/" + policyToString(pol));

    // Unique report file per instance
    std::string log_filepath = base_path + "/all_checkpoint_fronts.csv";
    std::ofstream log_file(log_filepath);

    if(!log_file.is_open()){
        std::cerr << "Error: Could'nt open the stats file: " << log_filepath << std::endl;
        return;
    }
    // Header
    log_file << "Seed,Generation,Policy,Rank,Time_Fitness,Energy_Fitness,Crossover_P,Mutation_P\n";
    
    // Seed loop
    for(int seed : seeds){
        std::cout << "[" << instance_name << "] Seed: " << seed << "...\n";
        gen.seed(seed);
        
        std::vector<Individual> population(population_size);
        //* Gen 0
        for(Individual& pop : population) pop = generateChromosome(data, gen);

        // Initial adaptive parameters
        AdaptiveParams params = {80, 10}; 
        
        // To monitor improvement
        float global_best_makespan = 1e9;
        int gens_no_improve = 0;

        // Evaluating Gen 0
        for(const auto& pol : policies_order){
            PolicyType policy = pol.first;
            for(Individual& ind : population){
                auto [makespan, _] = totalTime(data, ind, policy, policies_order);
                float energy   = totalEnergy(data, ind, policy, policies_order);

                ind.time_fitness.setValue(policy, makespan);
                ind.energy_fitness.setValue(policy, energy);

                if(makespan < global_best_makespan) global_best_makespan = makespan;
            }
        }

        for(int generation = 0; generation < generation_size; ++generation){
            
            // Autoadapation logic
            if(gens_no_improve > 5){
                // Exploration
                params.crossover_prob = 60;
                params.mutation_prob = 20; 
            }else if(gens_no_improve == 0){
                // Exploitation
                params.crossover_prob = 90;
                params.mutation_prob = 1;
            }else{
                // Balanced
                params.crossover_prob = 80;
                params.mutation_prob = 10;
            }

            // Offsprings. Sort the current population
            //* Could be more efficient, but it's late and I'm tired
            for(const PolicyType& pol : policy_index_map){
                auto fronts = fastNonDominatedSort(population, pol);

                for(auto& front : fronts){
                    std::vector<Individual> temp_front;
                    for(int idx : front){
                        temp_front.push_back(population[idx]);
                    }
                    calculateCrowdingDistance(temp_front, pol);

                    for(size_t i = 0; i < front.size(); ++i){
                        int original_idx = front[i];
                        float new_cd = temp_front[i].crowding_distance.getValue(pol);
                        population[original_idx].crowding_distance.setValue(pol, new_cd);
                    }
                }
            }

            std::vector<Individual> offspring_population;
            while(offspring_population.size() < population_size){
                // Crossover
                int idx1 = random_parent(gen);
                int idx2 = random_parent(gen);
                
                Individual& parent1 = population.at(idx1);
                Individual& parent2 = population.at(idx2);

                auto [child1, child2] = crossover(parent1, parent2, policies_order, gen, params.crossover_prob);

                // Mutation with adaptive probability
                if(prob_gen(gen) < params.mutation_prob){
                    int type = std::uniform_int_distribution<>(0, 2)(gen);
                    if(type == 0) equitativeExchange(child1, gen);
                    else if(type == 1) interChrome(child1, gen);
                    else circular(child1, gen);
                }
                if(prob_gen(gen) < params.mutation_prob){
                    int type = std::uniform_int_distribution<>(0, 2)(gen);
                    if(type == 0) equitativeExchange(child2, gen);
                    else if(type == 1) interChrome(child2, gen);
                    else circular(child2, gen);
                }

                offspring_population.push_back(child1);
                if(offspring_population.size() < population_size){
                    offspring_population.push_back(child2);
                }
            }
            
            // Evaluation offspring
            float current_gen_best = 1e9;
            for(const auto& policy : policy_index_map){
                PolicyType pol = policy;
                for(Individual& child : offspring_population){
                    auto [makespan, _] = totalTime(data, child, pol, policies_order);
                    float energy   = totalEnergy(data, child, pol, policies_order);
                    
                    child.time_fitness.setValue(policy, makespan);
                    child.energy_fitness.setValue(policy, energy);
                    
                    if(makespan < current_gen_best) current_gen_best = makespan;
                }
            }
            
            // Check Improvement
            if(current_gen_best < global_best_makespan){
                global_best_makespan = current_gen_best;
                gens_no_improve = 0;
            } else {
                gens_no_improve++;
            }

            // Merge & Survival
            std::vector<Individual> combined_population = population;
            combined_population.insert(combined_population.end(), offspring_population.begin(), offspring_population.end());

            // Sort mixed population
            for(const PolicyType& pol : policy_index_map){
                auto fronts = fastNonDominatedSort(combined_population, pol);
                for(auto& front : fronts){
                    std::vector<Individual> temp_front;
                    for(int idx : front){
                        temp_front.push_back(combined_population[idx]);
                    }
                    calculateCrowdingDistance(temp_front, pol);
                    for(size_t i = 0; i < front.size(); ++i){
                        int original_idx = front[i];
                        float new_cd = temp_front[i].crowding_distance.getValue(pol);
                        combined_population[original_idx].crowding_distance.setValue(pol, new_cd);
                    }
                }
            }

            std::vector<Individual> next_population;
            while(next_population.size() < population_size){
                Individual survivor = tournamentSelection(combined_population, gen);
                next_population.push_back(survivor);
            }            
            population = next_population;

            // Save stats every 20th gen AND INCLUDE ADAPTIVE PARAMS
            if((generation + 1) % 20 == 0 || generation == 0){
                for(const auto& policy : policy_index_map){
                    auto fronts_indices = fastNonDominatedSort(population, policy);
                    if(fronts_indices.empty()) continue;

                    // The first front is saved
                    const std::vector<int>& pareto_indices = fronts_indices[0];
                    if(pareto_indices.empty()) continue;

                    for(int idx : pareto_indices){
                        const Individual& ind = population[idx];
                        log_file << seed << ","
                                 << (generation + 1) << ","
                                 << policyToString(policy) << ","
                                 << 1 << "," // Rank 1
                                 << ind.time_fitness.getValue(policy) << ","
                                 << ind.energy_fitness.getValue(policy) << ","
                                 << params.crossover_prob << "," 
                                 << params.mutation_prob << "\n";
                    }
                }
            }
        } // End Gen Loop

        // Save Gantt
        for(auto policy : policy_index_map){
            std::string policy_name = policyToString(policy);
            auto final_front = fastNonDominatedSort(population, policy);

            if(final_front.empty()) continue;

            const std::vector<int>& pareto_indices = final_front[0];
            std::vector<Individual> pareto_front_copies;
            for(int idx : pareto_indices) pareto_front_copies.push_back(population[idx]);

            calculateCrowdingDistance(pareto_front_copies, policy);
            // Sort by time to be consistent
             std::sort(pareto_front_copies.begin(), pareto_front_copies.end(), [&](const Individual& a, const Individual& b){
                return a.time_fitness.getValue(policy) < b.time_fitness.getValue(policy);
            });

            int sol_idx = 1;
            for(Individual& ind : pareto_front_copies){
                auto [makespan, gantt] = totalTime(data, ind, policy, policies_order); 
                
                std::string gantt_filename = base_path + "/" +
                                             policy_name +
                                             "/seed_" + std::to_string(seed) +
                                             "_solution_" + std::to_string(sol_idx++) + 
                                             ".txt";
                saveGanttToFile(gantt_filename, gantt);
            }
        }
    } // End Seed Loop
    log_file.close();
    std::cout << "[" << instance_name << "] DONE.\n";
}