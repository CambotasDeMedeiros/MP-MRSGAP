#pragma once
#include "../include/data.h"
#include "../include/solveProblems.h"
#include "../include/preprocessing.h"
#include <unordered_map>
#include <limits>
#include <string>
#include <iomanip>
#include <optional>

void printCurrentTime(std::optional<std::chrono::steady_clock::time_point> start = std::nullopt);

int mainAlgorithm(const std::string instanceName, const double maxOptimizationTime, const bool writeLogFile, const bool printProblem, const double UB = +INFINITY, const bool toPrune = true, const bool warmStart = true, const bool tabuSearchW = false);

bool doesOverbookingExist(CPLEXMasterSolution solution, MasterData data);

int hardCapacityAlgorithm(const std::string instanceName);