#ifndef READ_TEST_H
#define READ_TEST_H

#pragma once

#include <vector>
#include <fstream>

std::vector<std::vector<float>> loadTime(std::ifstream& f);
std::vector<std::vector<float>> loadEnergy(std::ifstream& f);
std::vector<std::vector<int>> loadWork(std::ifstream& f);

#endif //READ_TEST_H