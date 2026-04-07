#include "../include/mainAlgorithm.h"

void printCurrentTime(std::optional<std::chrono::steady_clock::time_point> start){
    if (start.has_value()) {
        using clock = std::chrono::steady_clock;

        auto now = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - *start);

        auto hours = std::chrono::duration_cast<std::chrono::hours>(elapsed);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(elapsed - hours);
        auto seconds = elapsed - hours - minutes;

        std::cout << "Elapsed time: "
            << std::setw(2) << std::setfill('0') << hours.count() << ":"
            << std::setw(2) << std::setfill('0') << minutes.count() << ":"
            << std::setw(2) << std::setfill('0') << seconds.count()
            << "\n";
    }
    else {
        using clock = std::chrono::system_clock;

        auto now = clock::now();
        std::time_t t = clock::to_time_t(now);

        std::tm local{};
        #ifdef _WIN32
            localtime_s(&local, &t);
        #else
            localtime_r(&t, &local);
        #endif


        std::cout << "Current time: "
            << std::setw(2) << std::setfill('0') << local.tm_hour << ":"
            << std::setw(2) << std::setfill('0') << local.tm_min << ":"
            << std::setw(2) << std::setfill('0') << local.tm_sec
            << "\n";
    }
}

bool doesOverbookingExist(CPLEXMasterSolution solution, MasterData data) {
    for (int i = 0; i < data.nN; i++) {
        int transportBooking = 0;
        for (int r = 0; r < data.nR; r++) {
            if (abs(solution.x[i][r] - 1) < EPSILON) {
                transportBooking += data.q[i][r][0];
            }
        }
        if (transportBooking > data.Q[i][0]) {
            return true;
        }
    }
    return false;
}

int hardCapacityAlgorithm(const std::string instanceName) {
    //Starts counting the time.
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    //--------------------------------
    //------Algorithm Parameters------
    //--------------------------------
    std::filesystem::path exec_path = std::filesystem::current_path();
    std::filesystem::path filename = exec_path / "instances" / (instanceName + ".dat"); //Instance File

    //--------------------------------
    //----------Prepare data----------
    //--------------------------------
    ProblemData problemdata = readParametersFromFile(filename);
    //printProblemData(problemdata);
    if (problemdata.nN == 0) {
        std::cerr << "Error reading problem data. Exiting." << std::endl;;
        return 1;
    }

    MasterData masterdata = convertDataFromProblemToMaster(problemdata);

    
	CPLEXMasterSolution solution = solveHardMaster(masterdata);
    if (solution.feasible == 0) {
        std::cout << "Hard master problem infeasible. Exiting...\n";
        return 1;
    }
    else {
        writeHardMasterSolution(solution, masterdata, instanceName, start);
    }
    return 0;
}

int mainAlgorithm(const std::string instanceName, const double maxOptimizationTime, const bool writeLogFile, const bool printProblem, const double UB, const bool toPrune, const bool warmStart, const bool tabuSearchWS) {

    printCurrentTime();
     
    //Starts counting the time.
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    //--------------------------------
    //------Algorithm Parameters------
    //--------------------------------
    std::filesystem::path exec_path = std::filesystem::current_path();
    std::filesystem::path filename = exec_path / "instances" / (instanceName + ".dat"); //Instance File

    //--------------------------------
    //----------Prepare data----------
    //--------------------------------
    ProblemData problemdata = readParametersFromFile(filename);
    if (problemdata.nN == 0) {
        std::cerr << "Error reading problem data. Exiting.\n";
        return 1;
    }
    MasterData masterdata = convertDataFromProblemToMaster(problemdata);

    //-------------------------------
    //------Initialize Log File------
    //-------------------------------
    Logger logger(instanceName, writeLogFile);
    logger.log("INSTANCE: " + instanceName + "\n");
    

    //----------------------------------------------
    //---------Nested Benders Decomposition---------
    //----------------------------------------------

    //----------------------------------
    //--------------Set up--------------
    //----------------------------------
    std::cout << "\nStarting Nested Benders Decomposition (Fast-forward Heuristic)...\n";
    //std::map<int, std::vector<std::unique_ptr<Problem>>> problemsDict;
    std::vector<std::unique_ptr<Problem>> problemsVector;
    std::vector<std::string> problemsVectorIds;

    //-------------------------------
    //-------Write In Log File-------
    //-------------------------------
    logger.log("\nStarting Nested Benders Decomposition (Fast-forward Heuristic)...\n");

    //Creating master problem
    problemsVector.push_back(std::make_unique<MasterProblem>());
    problemsVectorIds.push_back("0");

    Problem* masterProblemPtr = problemsVector.back().get();
    masterProblemPtr->setLevel(0);
    masterProblemPtr->setScenario(0);

    auto* masterMasterProblemPtr = dynamic_cast<MasterProblem*> (masterProblemPtr);
    masterMasterProblemPtr->setMasterData(masterdata);

    std::vector <Problem*> nextBranch;

    CPLEXMasterSolution currentSolution;
    std::vector<std::unique_ptr<Problem>> bestTree;
    std::vector<std::vector<double>> bestX;
    int bestIteration = 0;

    std::vector<double> listUpperBound = {INFINITY};
    std::vector<double> listLowerBound = {-INFINITY};
    double optGap = INFINITY;

    int iteration = 0;
    int prunedSolutions = 0;
    bool stopOptimization = false;

    //---------------------------------
    //------------Algorithm------------
    //---------------------------------
    while (true && not stopOptimization) {

        //Set up levels.
        int i = 0;
        std::vector <Problem*> prevLevel = { };
        std::vector <Problem*> currLevel = { masterProblemPtr };
        std::vector <Problem*> nextLevel = {};

        printCurrentTime(start);
        std::cout << "\n===ITERATION " << iteration + 1 << "===\n\n";

        //-------------------------------
        //-------Write In Log File-------
        //-------------------------------
        logger.log("\n===ITERATION " + std::to_string(iteration + 1) + "===\n\n");

        //--------------------------------
        //----------Solve master----------
        //--------------------------------
        masterMasterProblemPtr->isProblemDifferent = 1;
        masterdata = masterMasterProblemPtr->masterData;

        if (warmStart && iteration == 0) {
            masterProblemPtr->problemUpperBound = UB;

            if (tabuSearchWS) {
                currentSolution = solveMaster(masterdata, writeLogFile, logger, masterProblemPtr->problemLowerBound, masterProblemPtr->problemUpperBound, true);
            }
            else {
                currentSolution = solveHardMaster(masterdata);
            }
        }
        else {
            currentSolution = solveMaster(masterdata, writeLogFile, logger, masterProblemPtr->problemLowerBound, masterProblemPtr->problemUpperBound);
        }

        auto* masterMasterProblemPtr = dynamic_cast<MasterProblem*>(masterProblemPtr);
        masterMasterProblemPtr->setMasterSolution(currentSolution);

        //Comment this if you know that the instance has a feasible solution.
        //if (iteration == 0) {
        //    if (not solveHardMaster(masterdata).feasible) {
        //        std::cout << "Hard master problem infeasible. Exiting...\n";
        //        return 1;
        //    }
        //}

        //--------------------------------
        //--------Check feasibility-------
        //--------------------------------
        if (currentSolution.feasible == 0) {
            std::cout << "Master problem infeasible. Exiting...\n";
            return 1;
        }

        //---------------------------------
        //---------Worthy Solutions--------
        //---------------------------------
        if (toPrune) {
            double subproblemTreeLB = solveSubproblemTreeLB(problemdata, currentSolution.x);
            masterProblemPtr->problemUpperBound = std::min(currentSolution.objective, masterProblemPtr->problemUpperBound);
            bool tempConvergence = std::abs(masterProblemPtr->problemUpperBound - masterProblemPtr->problemLowerBound) < EPSILON;

            logger.log("Problem ID = 0");
            if (currentSolution.feasible == 1) {
                logger.log(" z* =\t" + std::to_string(masterMasterProblemPtr->masterSol.objective));
            }
            else {
                logger.log(" z* =\tinfeasible");
            }
            logger.log("\nStochastic tree LB cost: " + std::to_string(subproblemTreeLB));
            logMasterSolution(currentSolution, logger);

            while (subproblemTreeLB == -INFINITY || (subproblemTreeLB != -INFINITY && currentSolution.objective - subproblemTreeLB < masterProblemPtr->problemLowerBound - EPSILON && not tempConvergence)) {

                if (subproblemTreeLB != -INFINITY) {
                    logger.log("Rejecting solution since it cannot improve the current problem lower bound ( " + std::to_string(currentSolution.objective - subproblemTreeLB) + " < " + std::to_string(masterProblemPtr->problemLowerBound) + " )\n\n\n");
                    //std::cout << "Rejecting solution since it cannot improve the current problem lower bound: " << prunedSolutions + 1 << "\n";
                }
                else {
                    logger.log("Rejecting solution because at least one the stochastic problems is infeasible.\n\n\n");
                    //std::cout << "Rejecting solution because at least one the stochastic problems is infeasible: " << prunedSolutions + 1 << "\n";
                }

                masterMasterProblemPtr->masterData.generalxArtificial.push_back(masterMasterProblemPtr->masterSol.x);
                masterdata = masterMasterProblemPtr->masterData;
                iteration++;
                prunedSolutions++;

                //std::cout << "\n===ITERATION " << iteration + 1 << "===\n\n";
                logger.log("\n===ITERATION " + std::to_string(iteration + 1) + "===\n\n");
                currentSolution = solveMaster(masterdata, writeLogFile, logger, masterProblemPtr->problemLowerBound, masterProblemPtr->problemUpperBound);
                masterMasterProblemPtr->setMasterSolution(currentSolution);
                subproblemTreeLB = solveSubproblemTreeLB(problemdata, currentSolution.x);

                masterProblemPtr->problemUpperBound = std::min(currentSolution.objective, masterProblemPtr->problemUpperBound);
                tempConvergence = std::abs(masterProblemPtr->problemUpperBound - masterProblemPtr->problemLowerBound) < EPSILON || bestX == currentSolution.x;
                if (tempConvergence) {
                    break;
                }
                logger.log("Problem ID = 0");
                if (currentSolution.feasible == 1) {
                    logger.log(" z* =\t" + std::to_string(masterMasterProblemPtr->masterSol.objective));
                }
                else {
                    logger.log(" z* =\tinfeasible");
                }
                logger.log("\nStochastic tree LB cost: " + std::to_string(subproblemTreeLB));
                logMasterSolution(currentSolution, logger);
            }
        }
        
        //---------------------------------
        //----------Print Problem----------
        //---------------------------------
        if (printProblem) {
            std::cout << "Problem ID = 0";
            if (currentSolution.feasible == 1) {
                std::cout << " z* =\t" << masterMasterProblemPtr->masterSol.objective;
            }
            else {
                std::cout << " z* =\tinfeasible";
            }
        }

        //-------------------------------
        //-------Write In Log File-------
        //-------------------------------
        logger.log("Problem ID = 0");
        if (currentSolution.feasible == 1) {
            logger.log(" z* =\t" + std::to_string(masterMasterProblemPtr->masterSol.objective));
        }
        else {
            logger.log(" z* =\tinfeasible");
        }

        //---------------------------------
        //-----------Upper Bound-----------
        //---------------------------------
        double masterProblemContribution = currentSolution.objective;
        if (warmStart && iteration == 0) {
            masterProblemPtr->problemUpperBound = UB;
        }
        else {
            masterProblemPtr->problemUpperBound = std::min(currentSolution.objective, masterProblemPtr->problemUpperBound);
        }
        listUpperBound.push_back(masterProblemPtr->problemUpperBound);
        listLowerBound.push_back(masterProblemPtr->problemLowerBound);
        optGap = std::round(std::abs((masterProblemPtr->problemUpperBound - masterProblemPtr->problemLowerBound) / (std::abs(masterProblemPtr->problemUpperBound)) * 10000)) / 100;

        //--------------------------------
        //----------Print Bounds----------
        //--------------------------------
        if (printProblem) {
            std::cout << "\t\tLB = " << masterProblemPtr->problemLowerBound << "\tUB = " << masterProblemPtr->problemUpperBound << "\n" << std::endl;
        }

        std::cout << "Optimality Gap: " << optGap << "%\n\n\n" << std::endl;

        //-------------------------------
        //-------Write In Log File-------
        //-------------------------------
        logger.log("\t\tLB =\t" + std::to_string(masterProblemPtr->problemLowerBound) + "\tUB =\t" + std::to_string(masterProblemPtr->problemUpperBound) + "\n");
        //logger.log("Stochastic tree LB cost: " + std::to_string(subproblemTreeLB));	
        logMasterSolution(currentSolution, logger);
        //if (writeLogFile) {
        //    for (int iIndex = 0; iIndex < masterdata.nN; iIndex++) {
        //        int booked = 0;
        //        for (int rIndex = 0; rIndex < masterdata.nR; rIndex++) {
        //            if (std::abs(currentSolution.x[iIndex][rIndex] - 1) < EPSILON) {
        //                booked += masterdata.q[rIndex][0];
        //            }
        //        }
        //        logger.log("Transport " + std::to_string(iIndex + 1) + " = " + std::to_string(booked) + "\n");
        //    }
        //}

        if (masterProblemPtr->problemUpperBound + EPSILON < masterProblemPtr->problemLowerBound) {
            logger.log("\n\nERROR: LB > UB\n\n");
            std::cout << "\n\nERROR: LB > UB\n\n";
            return 1;
        }

        //-------------------------------
        //-------Write In Log File-------
        //-------------------------------
        logger.log("Optimality Gap: " + std::to_string(optGap) + "%\n\n\n");

        //---------------------------------
        //-----------Print Level-----------
        //---------------------------------
        if (printProblem) {
            std::cout << "Current Level: ";
            for (const auto& group : currLevel) {
                std::cout << group->getProblemID() << " ";
            }
        }
		std::cout << "\n\n";

        //-------------------------------
        //-------Write In Log File-------
        //-------------------------------
        if (writeLogFile) {
            logger.log("Current Level: ");
            for (const auto& group : currLevel) {
                logger.log(group->getProblemID() + " ");
            }
            logger.log("\n\n");
        }

        //--------------------------------
        //------Check for convergence-----
        //--------------------------------
        if (std::abs(masterProblemPtr->problemUpperBound - masterProblemPtr->problemLowerBound) < EPSILON || bestX == currentSolution.x)
        {
            break;
        }

        //--------------------------------
        //----------Prepare data----------
        //--------------------------------
        //SubproblemData subproblemdata = preprocessSubproblemData(problemdata, currentSolution.x);
        auto subproblemdata = std::make_shared<const SharedSubproblemData>(
            preprocessSubproblemData(problemdata, currentSolution.x)
        );

        masterMasterProblemPtr->masterData.nChildren = subproblemdata->nScenariosPerAllo[0];
        masterMasterProblemPtr->masterData.nTotalChildren += subproblemdata->nScenariosPerAllo[0];
        

       //---------------------------------
       //--------Check Overbooking--------
       //---------------------------------
       // Comment this part if you want to write each scneario in the solution summary
       //\exist i \in N: \sum_{r \in R} x_{ir} q_r \geq Q_i OR \exist r \in R: y_r != 0
       //if (iteration == 0 && not doesOverbookingExist(currentSolution, masterdata)) {
       //    //Not accouting for overbooking and underbooking costs
       //    //double totalSubproblemContribution = 0;
       //    //for (int i = 0; i < subproblemdata.nN; i++) {
       //    //    for (int s = 0; s < subproblemdata.nScenariosPerLvl[i]; s++) {
       //    //        totalSubproblemContribution += subproblemdata.e * subproblemdata.rho[i][s] * (subproblemdata.Q[i][0] - subproblemdata.xi[i][s][0]);
       //    //    }
       //    //}
       //    //auto* masterMasterProblemPtr = dynamic_cast<MasterProblem*>(masterProblemPtr);
       //    //masterMasterProblemPtr->masterSol.objective = masterMasterProblemPtr->masterSol.objective - totalSubproblemContribution;
       //    //problemsDict[iteration] = { std::move(problemsVector) };
       //    bestTree = std::move(problemsVector);
       //    break;
       //}

       bool firstTimeInFirstLevel = true;
       bool isProblemInfeasibleInFirstLevel = false;
       //--------------------------------
       //--------While not Master--------
       //--------------------------------
        while (i > -1 && not stopOptimization)
        {   
            bool isProblemInfeasibleInCurrentLevel = false;
            bool isProblemNotOptimalInCurrentLevel = false;
            int p = 1;
            int nEqualSolutions = 0;
            int nInfeasibleSolutions = 0;
            double sumLowerBoundFirst = 0.0;
            double sumUpperBoundFirst = 0.0;
            bool toPrune = false;

            for (auto parentProblemPtr : currLevel) {

                //---------------------
                //--Stop Optimizaiton--
                //---------------------
                if (stopOptimization) {
                    break;
                }

                nextBranch.clear();
                int s = 0;
                double childrenContribution = 0;
                bool calcUpperBound = 1;

                for (int j = 1; j <= subproblemdata->nScenariosPerAllo[i]; j++) {

                    //---------------------
                    //--Stop Optimizaiton--
                    //---------------------
                    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                    std::chrono::duration<double, std::milli> elapsed = now - start;
                    double elapsedTime = elapsed.count() / 1000.0;
                    if (elapsedTime > maxOptimizationTime) {
                        stopOptimization = true;
                        break;
                    }

                    if (stopOptimization) {
                        std::cout << "Maximum Run Time Reached...\n";
                        logger.log("Maximum Run Time Reached...\n");
                    }

                    //---------------------------------
                    //-----Creating the subproblem-----
                    //---------------------------------
                    Problem* currentProblemPtr = nullptr;
                    std::string currentProblemID = std::to_string(i + 1) + "_" + std::to_string(p);
                    SubproblemData currentSubproblemData;

                    //Check if the subproblem exists.
                    auto it = std::find(problemsVectorIds.begin(), problemsVectorIds.end(), currentProblemID);
                    if (it != problemsVectorIds.end()) {
                        // ID exists
                        currentProblemPtr = getProblemByID(problemsVector, currentProblemID);
                        auto* currentSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                        currentSubproblemData = currentSubproblemPtr->subproblemData;
                    }
                    else {
                        // ID does not exist  create new Subproblem
                        //problemsVector.push_back(std::make_unique<Subproblem>(currentProblemID));
                        //currentProblemPtr = problemsVector.back().get();
                        //problemsVectorIds.push_back(currentProblemID);
                        //currentSubproblemData.shared = subproblemdata;                       
                        //currentProblemPtr->setLevel(i + 1);
                        //currentProblemPtr->setScenario(p);
                        //parentProblemPtr->addChildProblem(currentProblemPtr);
                        //currentProblemPtr->setParentProblem(parentProblemPtr);
                        //currentSubproblemData.iStar = i + 1;
                        //currentSubproblemData.sStar = p;
                        //currentSubproblemData.nS = subproblemdata->nScenariosPerAllo[i];
                        //
                        //auto* currentSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                        //currentSubproblemPtr->setSubproblemData(currentSubproblemData);


                        currentSubproblemData.shared = subproblemdata;
                        currentSubproblemData.iStar = i + 1;
                        currentSubproblemData.sStar = p;
                        currentSubproblemData.nS = subproblemdata->nScenariosPerAllo[i];

                        problemsVector.push_back(std::make_unique<Subproblem>(currentProblemID, currentSubproblemData));
                        currentProblemPtr = problemsVector.back().get();
                        problemsVectorIds.push_back(currentProblemID);

                        currentProblemPtr->setLevel(i + 1);
                        currentProblemPtr->setScenario(p);
                        currentProblemPtr->setParentProblem(parentProblemPtr);
                        parentProblemPtr->addChildProblem(currentProblemPtr);

						//std::cout << "Created subproblem with ID: " << currentProblemID << "\n";
						//std::cout << "xi: " << currentSubproblemData.shared->xi[currentSubproblemData.iStar - 1][currentSubproblemData.sStar - 1][0] << "\n";
                        //std::cout << "rho: " << currentSubproblemData.shared->rho[currentSubproblemData.iStar - 1][currentSubproblemData.sStar - 1] << "\n";

                        auto* currentSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                        //currentSubproblemPtr->setSubproblemData(currentSubproblemData);
                    }

                    //-------------------------------------
                    //--Check if the problem is different--
                    //-------------------------------------
                    if (not parentProblemPtr->isProblemDifferent) {
                        currentProblemPtr->isProblemDifferent = 0;
                    }
                    if (parentProblemPtr->isProblemDifferent && parentProblemPtr->problemID != "0") {
                        currentProblemPtr->isProblemDifferent = 1;
                    }
                    if (not currentProblemPtr->isProblemDifferent) {
                        nextBranch.push_back(currentProblemPtr);
                        p++;
                        s++;
                        nEqualSolutions++;
                        continue;
                    }

                    //----------------------------------
                    //----Pass down parent solutions----
                    //----------------------------------
                    auto* parentSubproblemPtr = dynamic_cast<Subproblem*>(parentProblemPtr);

                    if (i > 0 && parentSubproblemPtr->subproblemSol.feasible == 0) {
                        auto* currentSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                        currentSubproblemPtr->subproblemSol.feasible = 0;
                        nextBranch.push_back(currentProblemPtr);
                        isProblemInfeasibleInCurrentLevel = true;
                        nInfeasibleSolutions++;
                        p++;
                        s++;
                        continue;
                    }

                    if (i > 0 && parentSubproblemPtr->subproblemSol.feasible == 1) {
                        //currentSubproblemData.shared->xParent = parentSubproblemPtr->subproblemData.shared->xParent;
                        currentSubproblemData.wParent = parentSubproblemPtr->subproblemData.wParent;
                        //currentSubproblemData.zParent = parentSubproblemPtr->subproblemData.zParent;
                        currentSubproblemData.wParent.push_back(parentSubproblemPtr->subproblemSol.w);
                        //currentSubproblemData.zParent.push_back(parentSubproblemPtr->subproblemSol.z);
                        currentProblemPtr->isProblemDifferent = 1;
                    }

                    //---------------------------------
                    //--------Check convergence--------
                    //---------------------------------
                    bool convergence = std::fabs(currentProblemPtr->problemUpperBound - currentProblemPtr->problemLowerBound) < EPSILON;
                    //bool convergence = std::fabs(currentProblemPtr->problemUpperBound - currentProblemPtr->problemLowerBound)/(std::max(std::max(std::abs(currentProblemPtr->problemUpperBound), std::abs(currentProblemPtr->problemLowerBound)), smallThreshold)) < EPSILON;
                    if (convergence && i != subproblemdata->nN - 1) {
                        for (auto* childPtr : currentProblemPtr->childrenProblems) {
                            bool childNotConverged = std::fabs(childPtr->problemUpperBound - childPtr->problemLowerBound) > EPSILON;
                            //bool childNotConverged = std::fabs(childPtr->problemUpperBound - childPtr->problemLowerBound) / (std::max(std::max(std::abs(currentProblemPtr->problemUpperBound), std::abs(currentProblemPtr->problemLowerBound)), smallThreshold)) > EPSILON;
                            auto* childSubproblemPtr = dynamic_cast<Subproblem*>(childPtr);
                            bool childInfeasible = !childSubproblemPtr->subproblemSol.feasible;
                            if (childInfeasible || childNotConverged) {
                                convergence = false;
                                break;
                            }
                        }
                    }

                    //--------------------------------
                    //--------Solve subproblem--------
                    //--------------------------------
                    auto* currentSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                    CPLEXSubproblemSolution subproblemSolution = currentSubproblemPtr->subproblemSol;
                    if (not convergence) {
                        logSubproblemImportedVariables(currentSubproblemData, logger);
                        subproblemSolution = solveSubproblem(currentSubproblemData, writeLogFile, logger);
                        currentSubproblemPtr->setSubproblemSolution(subproblemSolution);
                        currentSubproblemPtr->setSubproblemData(currentSubproblemData);
                        //printSubproblemSolution(subproblemSolution);


                        //---------------------------------
                        //-------Setting Lower Bound-------
                        //---------------------------------
                        if (currentSubproblemPtr->subproblemSol.feasible == 1) {
                            currentProblemPtr->problemLowerBound = std::max(currentSubproblemPtr->subproblemSol.objective, currentProblemPtr->problemLowerBound);
                        }
                        else {
                            currentProblemPtr->problemUpperBound = +INFINITY;
                            currentProblemPtr->problemLowerBound = -INFINITY;
                        }

                        //If problem is a leaf the solution is optimal.
                        if (i == subproblemdata->nN - 1 && currentSubproblemPtr->subproblemSol.feasible == 1) {
                            currentProblemPtr->problemLowerBound = currentSubproblemPtr->subproblemSol.objective;
                            currentProblemPtr->problemUpperBound = currentSubproblemPtr->subproblemSol.objective;
                            convergence = true;
                        }

                        if (currentSubproblemPtr->subproblemSol.feasible == 0) {
                            nInfeasibleSolutions++;
                            isProblemInfeasibleInCurrentLevel = true;
                            if (i == 0) {
                                isProblemInfeasibleInFirstLevel = true;
                            }
                        }

                        #pragma region Prints
                        
                        //---------------------------------
                        //----------Print Problem----------
                        //---------------------------------

                        if (printProblem) {
                            std::cout << "Problem ID = " << currentProblemID;
                            if (subproblemSolution.feasible == 1) {
                                std::cout << " z* =\t" << currentSubproblemPtr->subproblemSol.objective;
                            }
                            else {
                                std::cout << " z* =\tinfeasible";
                            }
                        }

                        //-------------------------------
                        //-------Write In Log File-------
                        //-------------------------------
                        logger.log("Problem ID = " + currentProblemID);
                        if (subproblemSolution.feasible == 1) {
                            logger.log(" z* =\t" + std::to_string(currentSubproblemPtr->subproblemSol.objective));
                        }
                        else {
                            logger.log(" z* =\tinfeasible");
                        }

                        //--------------------------------
                        //----------Print Bounds----------
                        //--------------------------------
                        if (printProblem) {
                            std::cout << "\t\tLB = " << currentProblemPtr->problemLowerBound << "\tUB = " << currentProblemPtr->problemUpperBound << std::endl;
                        }

                        //-------------------------------
                        //-------Write In Log File-------
                        //-------------------------------
                        double sumAlphas = 0.0;
                        for (double alpha_t : currentSubproblemPtr->subproblemSol.alpha) {
                            sumAlphas += alpha_t;
                        }
                        logger.log("\t\tLB =\t" + std::to_string(currentProblemPtr->problemLowerBound) + "\tUB =\t" + std::to_string(currentProblemPtr->problemUpperBound));
                        logSubproblemSolution(subproblemSolution, logger);

                        if (currentProblemPtr->problemUpperBound + EPSILON < currentProblemPtr->problemLowerBound) {
                            logger.log("\n\nERROR: LB > UB\n\n");
                            std::cout << "\n\nERROR: LB > UB\n\n";
                            return 1;
                        }
                        #pragma endregion

                        //if (currentSubproblemPtr->subproblemSol.feasible == 1 && i < currentSubproblemData.nN) {
                        //    nextBranch.push_back(currentProblemPtr);
                        //    p++;
                        //    s++;
                        //}

                    }
                    //else {
                    //    nextBranch.push_back(currentProblemPtr);
                    //    p++;
                    //    s++;
                    //}

                    nextBranch.push_back(currentProblemPtr);
                    p++;
                    s++;

                    //--------------------------------------
                    //--------Check Tree Exploration--------
                    //--------------------------------------
					//Prune the remaining subproblems if the current lower bound is optimal for the current problem
                    if ((i != 0 && i != subproblemdata->nN-1) || (i == 0 && not firstTimeInFirstLevel)) {
                    
                        if (subproblemSolution.objective > currentProblemPtr->problemUpperBound - EPSILON) {
                            
                            logger.log("Pruning remaining subproblems at problem " + currentProblemID + " since they cannot improve the current problem lower bound ( " + std::to_string(subproblemSolution.objective) + " > " + std::to_string(currentProblemPtr->problemUpperBound) + " )\n\n");
							convergence = true;
                    
                        }
                    }
                    //Set the last problem to infeasible, if the master solution is dominated by the lower bounds of the first level subproblems
                    if (i == 0 && not isProblemInfeasibleInCurrentLevel) {
                        sumLowerBoundFirst += currentProblemPtr->problemLowerBound;
                        sumUpperBoundFirst += currentProblemPtr->problemUpperBound;
                        double delta = 0.5;
                    
						
                        //if ((currentSolution.objective - sumLowerBoundFirst < masterProblemPtr->problemLowerBound + EPSILON or currentSolution.objective -  masterProblemPtr->problemUpperBound + EPSILON > sumUpperBoundFirst) && j == subproblemdata.nScenariosPerAllo[i]) {
                        if ((currentSolution.objective - sumLowerBoundFirst < masterProblemPtr->problemLowerBound + EPSILON || currentSolution.objective - masterProblemPtr->problemUpperBound > sumUpperBoundFirst + delta + EPSILON) && j == subproblemdata->nScenariosPerAllo[i]) {
                            logger.log("Pruning remaining subproblems at problem " + currentProblemID + " since they cannot improve the master problem upper bound ( " + std::to_string(currentSolution.objective - sumLowerBoundFirst) + " < " + std::to_string(masterProblemPtr->problemLowerBound) + " )\n\n");
                            //Small trick
                            subproblemSolution.feasible = 0;
                            currentSubproblemPtr->setSubproblemSolution(subproblemSolution);
                            toPrune = true;
                    
                            //if (currentSolution.objective - masterProblemPtr->problemUpperBound > sumUpperBoundFirst + delta + EPSILON) {
                            //    std::cout << "-----------------------------------------------------------------------------\n";
                            //    std::cout << "f(x) = " << currentSolution.objective << " U = " << masterProblemPtr->problemUpperBound << " U(x) = " << sumUpperBoundFirst + delta << "\n";
                            //    std::cout << "-----------------------------------------------------------------------------\n";
                            //}
                        }
                    }

                    //---------------------------------
                    //-----------Convergence-----------
                    //---------------------------------
                    //Check if the children subproblems have converged
                    if (convergence && i!=subproblemdata->nN-1) {
                        for (auto* childPtr : currentProblemPtr->childrenProblems) {
                            bool childNotConverged = std::fabs(childPtr->problemUpperBound - childPtr->problemLowerBound) > EPSILON;
                            //bool childNotConverged = std::fabs(childPtr->problemUpperBound - childPtr->problemLowerBound) / (std::max(std::max(std::abs(currentProblemPtr->problemUpperBound), std::abs(currentProblemPtr->problemLowerBound)), smallThreshold)) > EPSILON;
					    	auto* childSubproblemPtr = dynamic_cast<Subproblem*>(childPtr);
                            bool childInfeasible = !childSubproblemPtr->subproblemSol.feasible;
                            if (childInfeasible || childNotConverged) {
                                convergence = false;
                                break;
                            }
					    }
					}
                    
                    if (convergence) {
                        currentProblemPtr->isProblemDifferent = 0;
                        nEqualSolutions++;
                        childrenContribution += currentSubproblemPtr->subproblemSol.objective;
                    }
                    else {
                        currentProblemPtr->isProblemDifferent = 1;
                        isProblemNotOptimalInCurrentLevel = true;
                        calcUpperBound = 0;
                    
                        for (auto* childProblemPtr : currentProblemPtr->childrenProblems) {
                            childProblemPtr->problemUpperBound = +INFINITY;
                            childProblemPtr->problemLowerBound = -INFINITY;
                    
                            auto* childSubproblemPtr = dynamic_cast<Subproblem*>(childProblemPtr);
                    
                            childSubproblemPtr->subproblemData.nK = 0;
                            childSubproblemPtr->subproblemData.generalisChildFeasable.clear();
                            childSubproblemPtr->subproblemData.generalzChild.clear();
                    
                            childSubproblemPtr->subproblemData.generaluFound.clear();
                            childSubproblemPtr->subproblemData.generalvFound.clear();
                            childSubproblemPtr->subproblemData.generalwFound.clear();
                            childSubproblemPtr->subproblemData.generalzFound.clear();
                        }
                    }

                    //--------------------------------
                    //-------Pass up the values-------
                    //--------------------------------
                    //If the infeasibility is in a leaf or not in the first set of subproblems after the master problem
                    if ((i == subproblemdata->nN - 1 || (subproblemSolution.feasible == 0 && i != 0)) || (convergence && i != 0)) { 
                        auto* childSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                        CPLEXSubproblemSolution parentSolution = parentSubproblemPtr->subproblemSol;
                        CPLEXSubproblemSolution childSolution = childSubproblemPtr->subproblemSol;

                        if (subproblemSolution.feasible == 0) {
                            parentSubproblemPtr->subproblemData.isChildFeasable.clear();
                            parentSubproblemPtr->subproblemData.zChild.clear();
                           
                            parentSubproblemPtr->subproblemData.zChild.push_back(-1);
                        }
                        else {
                            parentSubproblemPtr->subproblemData.zChild.push_back(childSolution.objective);
                        }
                        parentSubproblemPtr->subproblemData.isChildFeasable.push_back(childSolution.feasible);
                        parentSubproblemPtr->subproblemData.wFound = parentSolution.w;
                        //parentSubproblemPtr->subproblemData.zFound = parentSolution.z;

                        if (parentSubproblemPtr->subproblemData.isChildFeasable.size() == subproblemdata->nScenariosPerAllo[i] || subproblemSolution.feasible == 0) {
                            
                            auto* parentSubproblemPtr = dynamic_cast<Subproblem*>(parentProblemPtr);
                            parentSubproblemPtr->subproblemData.nK += 1;
                            
                            parentSubproblemPtr->subproblemData.generalisChildFeasable.push_back(std::move(parentSubproblemPtr->subproblemData.isChildFeasable));
                            
                  
                            parentSubproblemPtr->subproblemData.generalzChild.push_back(std::move(parentSubproblemPtr->subproblemData.zChild));
                            parentSubproblemPtr->subproblemData.generaluFound.push_back(std::move(parentSubproblemPtr->subproblemData.uFound));
                            parentSubproblemPtr->subproblemData.generalvFound.push_back(std::move(parentSubproblemPtr->subproblemData.vFound));
                            parentSubproblemPtr->subproblemData.generalwFound.push_back(std::move(parentSubproblemPtr->subproblemData.wFound));
                            //parentSubproblemPtr->subproblemData.generalzFound.push_back(std::move(parentSubproblemPtr->subproblemData.zFound));
                            parentSubproblemPtr->subproblemData.isChildFeasable.clear();
                            parentSubproblemPtr->subproblemData.zChild.clear();
                            parentSubproblemPtr->subproblemData.uFound.clear();
                            parentSubproblemPtr->subproblemData.vFound.clear();
                            parentSubproblemPtr->subproblemData.wFound.clear();
                            //parentSubproblemPtr->subproblemData.zFound.clear();

                            //double maxZ = 0;
                            //for (std::vector objectiveValues : parentSubproblemPtr->subproblemData.generalzChild) {
                            //    for (double z : objectiveValues) {
                            //        if (maxZ < z) {
                            //            maxZ = z;
                            //        }
                            //    }
                            //}
                            //parentSubproblemPtr->subproblemData.CutBigM = maxZ;

                            std::vector<double> M_vector(subproblemdata->nScenariosPerAllo[i], 0.0);

                            for (const auto& objectiveValues : parentSubproblemPtr->subproblemData.generalzChild) {
                                for (size_t j = 0; j < objectiveValues.size(); ++j) {
                                    if (objectiveValues[j] > M_vector[j]) {
                                        M_vector[j] = objectiveValues[j];
                                    }
                                }
                            }

                            parentSubproblemPtr->subproblemData.CutBigMVector = M_vector;
                 
   
                        }
                        //If infeasible go up
                        if (subproblemSolution.feasible == 0) {
                            p += currentSubproblemData.shared->nScenariosPerAllo[i] - s;
                            isProblemInfeasibleInCurrentLevel = true;
                            break;
                        }
                    }
                    
                    //If the infeasibility is in the first set of subproblems after the master problem OR the subproblems are feasible
                    if (i == 0 && (convergence || subproblemSolution.feasible == 0)) {
                        auto* childSubproblemPtr = dynamic_cast<Subproblem*>(currentProblemPtr);
                        auto* parentMasterPtr = dynamic_cast<MasterProblem*>(parentProblemPtr);
                        CPLEXMasterSolution parentSolution = parentMasterPtr->masterSol;
                        CPLEXSubproblemSolution childSolution = childSubproblemPtr->subproblemSol;
                        
                        if (subproblemSolution.feasible == 0) {
                            parentMasterPtr->masterData.isChildFeasable.clear();
                            parentMasterPtr->masterData.zChild.clear();
                            parentMasterPtr->masterData.zChild.push_back(-1);
                        }
                        else {
                            parentMasterPtr->masterData.zChild.push_back(childSolution.objective);
                        }
                        parentMasterPtr->masterData.isChildFeasable.push_back(childSolution.feasible);

                        parentMasterPtr->masterData.xFound = parentSolution.x;

                        //If the subproblem is infeasible OR all the subproblems are feasible
                        if (parentMasterPtr->masterData.isChildFeasable.size() == subproblemdata->nScenariosPerAllo[i] || toPrune || subproblemSolution.feasible == 0) {
                            parentMasterPtr->masterData.nK += 1;
                            
                            parentMasterPtr->masterData.generalisChildFeasable.push_back(std::move(parentMasterPtr->masterData.isChildFeasable));
                            parentMasterPtr->masterData.generalzChild.push_back(std::move(parentMasterPtr->masterData.zChild));
                            parentMasterPtr->masterData.generalxFound.push_back(std::move(parentMasterPtr->masterData.xFound));
                            parentMasterPtr->masterData.isChildFeasable.clear();
                            parentMasterPtr->masterData.zChild.clear();
                            parentMasterPtr->masterData.xFound.clear();

                            //double maxZ = 0;
                            //for (std::vector objectiveValues : parentMasterPtr->masterData.generalzChild) {
                            //    for (double z : objectiveValues) {
                            //        if (maxZ < z) {
                            //            maxZ = z;
                            //        }
                            //    }
                            //}
                            //parentMasterPtr->masterData.CutBigM = maxZ;
                        }
                        //If infeasible go up
                        if (subproblemSolution.feasible == 0) {
                            p += currentSubproblemData.shared->nScenariosPerAllo[i] - s;
                            isProblemInfeasibleInCurrentLevel = true;
                            break;
                        }
                    }
                }
                nextLevel.insert(nextLevel.end(), nextBranch.begin(), nextBranch.end());

                //----------------------------------
                //-------Clear If Not Optimal-------
                //----------------------------------
                if (i != 0 && isProblemNotOptimalInCurrentLevel) {
                    auto* parentSubproblemPtr = dynamic_cast<Subproblem*>(parentProblemPtr);
                    parentSubproblemPtr->subproblemData.zChild.clear();
                    parentSubproblemPtr->subproblemData.isChildFeasable.clear();
                    parentSubproblemPtr->subproblemData.uFound.clear();
                    parentSubproblemPtr->subproblemData.vFound.clear();
                    parentSubproblemPtr->subproblemData.wFound.clear();
                    parentSubproblemPtr->subproblemData.zFound.clear();
                }

                //---------------------------------
                //-------Setting Upper Bound-------
                //---------------------------------
                if (i != 0 && (calcUpperBound == 1 || toPrune)) {
                    auto* parentSubproblemPtr = dynamic_cast<Subproblem*>(parentProblemPtr);
                    parentProblemPtr->problemUpperBound = std::min(parentSubproblemPtr->subproblemSol.objective + childrenContribution, parentProblemPtr->problemUpperBound);
                }
            }


            //If going down.
            if (i != subproblemdata->nN - 1 && not isProblemInfeasibleInCurrentLevel && nEqualSolutions != subproblemdata->nScenariosPerLvl[i] && not toPrune) {
            //if ((i != subproblemdata.nN - 1 && nEqualSolutions != subproblemdata.nScenariosPerLvl[i] - nInfeasibleSolutions) && not toPrune && not isProblemInfeasibleInCurrentLevel) {
				firstTimeInFirstLevel = false;
                prevLevel = currLevel;
                currLevel = nextLevel;
                nextLevel.clear();
                i++;
            }
            //If going up.
            else {
                currLevel = prevLevel;
                prevLevel.clear();
                if (currLevel[0]->getProblemID() != "0") {
                    for (auto currentProblemPrt : currLevel) {
                        Problem* parentProblemPrt = currentProblemPrt->getParentProblem();
                        auto it = std::find_if(prevLevel.begin(), prevLevel.end(),
                            [&](Problem* p) { return p->getProblemID() == parentProblemPrt->getProblemID(); });

                        if (it == prevLevel.end()) {
                            prevLevel.push_back(parentProblemPrt);
                        }
                    }
                }
                nextLevel.clear();
                i--;
            }
            
            //---------------------------------
            //-----------Print Level-----------
            //---------------------------------
            if (printProblem) {
                std::cout << "\nCurrent Level: ";
                for (const auto& group : currLevel) {
                    std::cout << group->getProblemID() << " ";
                }
                std::cout << "\n\n" << std::endl;
            }

            //-------------------------------
            //-------Write In Log File-------
            //-------------------------------
            if (writeLogFile) {
                logger.log("\nCurrent Level: ");
                for (const auto& group : currLevel) {
                    logger.log(group->getProblemID() + " ");
                }
                logger.log("\n\n");
            }

        }

        //----------------------------------
        //------Set Master Lower Bound------
        //----------------------------------
		auto copiedProblems = deepCopy(problemsVector);
        if (not isProblemInfeasibleInFirstLevel && not stopOptimization) {
            double subproblemContribution = 0;
            Problem& p = *problemsVector[0];
            for (size_t iIndex = 0; iIndex < p.childrenProblems.size(); iIndex++) {
                auto* childSubproblemPtr = dynamic_cast<Subproblem*>(p.childrenProblems[iIndex]);
                subproblemContribution += cleanDouble(childSubproblemPtr->subproblemSol.objective);
                //std::cout << "Child Objective: " << childSubproblemPtr->subproblemSol.objective << "\n";
            }
            if (masterProblemContribution - subproblemContribution > masterProblemPtr->problemLowerBound) {
                masterProblemPtr->problemLowerBound = masterProblemContribution - subproblemContribution;
                //std::cout << "Master: " << masterProblemContribution << " Subproblems: " << subproblemContribution << " Total: " << masterProblemContribution - subproblemContribution << "\n\n\n";
                bestIteration = iteration;
                //Write the current solution
                //writeFinalSolution(problemsVector, start, stopOptimization, optGap, instanceName, iteration, bestIteration);
                bestX = masterMasterProblemPtr->masterSol.x;
                bestTree = std::move(copiedProblems);
            }

        }

        //Save the problems created in a dict with jey equal to iteraation number.
        //auto copiedProblems = deepCopy(problemsVector);
        //problemsDict[iteration] = std::move(copiedProblems);
		copiedProblems.clear();
        problemsVector.resize(1);
        problemsVectorIds.resize(1);
        masterProblemPtr->childrenProblems.clear();        

        iteration++;


        //---------------------------------
        //--------Print Contribution-------
        //---------------------------------
        //if (printProblem) {
        //    double childrenContributionCurrentSolution = 0;
        //    for (double zChild : masterMasterProblemPtr->masterData.generalzChild.back()) {
        //        childrenContributionCurrentSolution += zChild;
        //    }
        //    std::cout << "Total Value Of Subproblems = " << childrenContributionCurrentSolution << "\n";
        //}

    }

    std::cout << "\nFinish Benders Decomposition...\n";

    //-------------------------------
    //-------Write In Log File-------
    //-------------------------------
    logger.log("\nFinish Benders Decomposition...\n");

    if (bestTree.empty()) {
        std::cout << "No feasible solution found.\n";
        writeFinalSolution(bestTree, start, stopOptimization, optGap, instanceName, iteration, bestIteration, prunedSolutions);
    }
    else {
        std::cout << "Feasible solution found.";
        writeFinalSolution(bestTree, start, stopOptimization, optGap, instanceName, iteration, bestIteration, prunedSolutions);
        writeBounds(listUpperBound, listLowerBound, instanceName);
        writeSolutionSummary(bestTree, instanceName);
    }

    printElapsedTime(start, "Finished");    
    
    problemsVector.clear();
    problemsVectorIds.clear();
    //problemsDict.clear();


    return 0;
}