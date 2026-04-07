#include "../include/mainAlgorithm.h"


int main()
{
	//--------------------------------
	//------Algorithm Parameters------
	//--------------------------------
	std::string instanceName = "test_benchmark_3_5_2-1";        //Instance data file name
    double maxOptimizationTime = 60 * 60 * 3;				    //Maximum Optimization Time
	bool writeLogFile = false;								    //To write the log file
	bool printProblem = false;								    //To write the each problem in the console
	double UB = +INFINITY;                                      //Inital upper bound for the optimization (if known)
	bool toPrune = true;										//To prune the master solution according to the pruning condition
	bool warmStart = true;									    //To warm start the optimization with an initial solution (if known)
	bool tabuSearchWS = true;									//If true, use the tabu search solution as initial solution for the optimization (if warmStart is true). If false, the initial solution is the MRGAP optimal solution (if warmStart is true).
    //Note that the tabu search solution needs to be added manually into the instance data file. A pyhton script is provided to do this automatically.

    #pragma region Parameter Validation
    tabuSearchWS = tabuSearchWS && warmStart && toPrune;
    std::string algorithmVersion;

    if (toPrune) {
        if (warmStart) {
            if (tabuSearchWS) {
                algorithmVersion = "NLBBD-P-TS\n";
            }
            else {
                algorithmVersion = "NLBBD-P-WS\n";
            }
        }
        else {
            algorithmVersion = "NLBBD-P-CS\n";
        }
    }
    else {
        if (warmStart) {
            algorithmVersion = "NLBBD-NP-WS\n";
        }
        else {
            algorithmVersion = "NLBBD-NP-CS\n";
        }
    }

    #pragma endregion
    //--------------------------------


	std::string instanceBaseName = "benchmark_";
    const int nInstances = 2;
    //const std::vector<int> N_values = {2, 2, 2, 2, 3, 3, 4, 4};
    //const std::vector<int> R_values = {5, 8, 11, 15, 8, 10, 7, 10};

    const std::vector<int> N_values = {3};
    const std::vector<int> R_values = {10};
    
    #pragma region Running the algorithm for multiple instances
    int j = 0;
    for (std::size_t i = 0; i < N_values.size(); ++i) {
        const int nNinstance = N_values[i];
        const int nRinstance = R_values[i];
    
        for (int h = 1; h <= 1; ++h) {
            for (int t = 2; t <= nInstances; ++t) {
                std::string instanceName;
    
                if (nRinstance <= 9) {
                    instanceName =
                        instanceBaseName +
                        std::to_string(nNinstance) + "_0" +
                        std::to_string(nRinstance) + "_" +
                        std::to_string(h) + "-" +
                        std::to_string(t);
                }
                else {
                    instanceName =
                        instanceBaseName +
                        std::to_string(nNinstance) + "_" +
                        std::to_string(nRinstance) + "_" +
                        std::to_string(h) + "-" +
                        std::to_string(t);
                }
    
                std::cout << "\n\n\nSolving instance: "
                    << instanceName << std::endl;
                std::cout << "Algortihm: " << algorithmVersion << std::endl; 
    
                hardCapacityAlgorithm(instanceName);
                mainAlgorithm(instanceName, maxOptimizationTime, writeLogFile, printProblem, UB, toPrune, warmStart, tabuSearchWS);
                j++;
            }
        }
    }
    #pragma endregion

	return 0;
}