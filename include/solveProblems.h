#pragma once
#include "data.h"
#include <ilcplex/ilocplex.h>
#include <vector>
#include <sstream>
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>

double cleanDouble(const double val);

CPLEXMasterSolution solveHardMaster(const MasterData& data);

CPLEXMasterSolution solveMaster(const MasterData& data, const bool writeLogFile, Logger& logger, const double& LB, const double& UB, const bool& wasGivenInitialSolution = false);

CPLEXSubproblemSolution solveSubproblem(const SubproblemData& data, const bool writeLogFile, Logger& logger);

void writeHardMasterSolution(const CPLEXMasterSolution& sol, const MasterData& data, const std::string& isntanceName, const std::chrono::steady_clock::time_point start);

void logMasterSolution(const CPLEXMasterSolution& sol, Logger& logger);

void logSubproblemSolution(const CPLEXSubproblemSolution& sol, Logger& logger);

void logSubproblemImportedVariables(const SubproblemData& data, Logger& logger);

double solveSubproblemTreeLB(const ProblemData& data, const std::vector<std::vector<double>> x);