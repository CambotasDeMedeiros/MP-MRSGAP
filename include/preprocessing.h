#pragma once
#include "../include/data.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <map>
#include <algorithm>
#include <numeric>
#include <vector>
#include <cmath>
#include <chrono>
#include <random>

struct ScenariosData {
    std::vector<std::vector<double>> rho;
    std::vector<std::vector<std::vector<int>>> xi;
    std::vector<std::vector<std::vector<int>>> gamma;
    int nH;
};

struct GPBDData {
    std::vector<double> probs;
    std::vector< std::vector<int>> weightsucess;
    std::vector< std::vector<int>> weightnotsucess;
    GPBDData() = default;
};

struct GeneralGPBDData {
    std::vector<GPBDData> allocations;
    int nH;
    GeneralGPBDData() = default;
};

ProblemData readParametersFromFile(const std::filesystem::path& filename);

MasterData convertDataFromProblemToMaster(const ProblemData& prob);

std::vector<std::tuple<std::vector<int>, std::vector<int>, double>> generalizedPoissonBinomialPMF(const GPBDData& gpbddata, const int nH);

ScenariosData getScenariosData(const GeneralGPBDData& data);

void updateSubproblemData(SharedSubproblemData& subproblem, const ScenariosData& scenarioDataAllo, const ScenariosData& scenarioDataLvl, const std::vector<std::vector<double>> x);

SharedSubproblemData convertDataFromProblemToSubproblem(const ProblemData& prob, const std::vector <std::vector<double>> x);

void printElapsedTime(const std::chrono::steady_clock::time_point start,
    const std::string& message);

GeneralGPBDData convertDataToGPBD(const std::vector <double> p, const std::vector < std::vector < std::vector<int>>> q, const std::vector<std::vector<double>> x, const int nH);

ScenariosData getBranchScenariosData(const ScenariosData& input);

SharedSubproblemData preprocessSubproblemData(const ProblemData& problem, const std::vector<std::vector<double>> x);

void writeFinalSolution(const std::vector<std::unique_ptr<Problem>>& problemTree, const std::chrono::steady_clock::time_point start, const bool stopOptimization, const double optGap, const std::string instanceName, const int iteration, const int bestIteration, const int prunedSolutions);

void writeBounds(const std::vector<double> upperBounds, const std::vector<double> lowerBounds, const std::string instanceName);

void writeSolutionSummary(const std::vector<std::unique_ptr<Problem>>& problemTree, const std::string instanceName);
