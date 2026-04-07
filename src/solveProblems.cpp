#include "../include/solveProblems.h"
const int nThreads = 1;

double cleanDouble(const double val) {
    if (std::abs(val) < NUMERICAL_ZERO) {
        return 0.0;
    }
    return val;
}

CPLEXMasterSolution solveHardMaster(const MasterData& data)
{
    // Model Definition
    IloEnv masterenv; // environment object
    IloModel model(masterenv);
    IloCplex cplex(model);
    
    CPLEXMasterSolution sol;
    
    try {
        //Decision Variables
        IloArray <IloBoolVarArray> x(masterenv, data.nN); // variable x binary
        for (int i = 0; i < data.nN; i++) {
            x[i] = IloBoolVarArray(masterenv, data.nR);
        }
    
        IloBoolVarArray y(masterenv, data.nR); // variable y binary
    
        for (int i = 0; i < data.nN; i++) {
            for (int r = 0; r < data.nR; r++) {
                std::string varName = "x(" + std::to_string(i + 1) + "," + std::to_string(r + 1) + ")";
                x[i][r].setName(varName.c_str());
            }
        }

        //-----------------------------------
        //------------Constraints------------
        //-----------------------------------
        //Every request is allocated.
        for (int r = 0; r < data.nR; r++) {
            IloExpr sum(masterenv);
            for (int i = 0; i < data.nN; i++) {
                sum += x[i][r];
            }
            model.add(sum == 1);
            sum.end();
        }
    
        //Maximum Capacity
        for (int h = 0; h < data.nH; h++) {
            for (int i = 0; i < data.nN; i++) {
                IloExpr cap(masterenv);
                for (int r = 0; r < data.nR; r++) {
                    cap += data.q[i][r][h] * x[i][r];
                }
                model.add(cap <= data.Q[i][h]);
                cap.end();
            }
        }
    
        ////Fix requests already allocated
        //for (int r = 0; r < data.nR; r++) {
        //    if (data.t[r] != 0) {
        //        model.add(x[data.t[r] - 1][r] == 1);
        //    }
        //}
        
        // Objective Function
        IloExpr objExpr(masterenv);
        for (int i = 0; i < data.nN; i++) {
            for (int r = 0; r < data.nR; r++) {
                objExpr += x[i][r] * data.a[i][r];
            }
        }
        //for (int i = 0; i < data.nN; i++) {
        //    objExpr -= data.e * data.Q[i][0];
        //    for (int r = 0; r < data.nR; r++) {
        //        objExpr += data.e * x[i][r] * data.q[i][r][0];
        //    }
        //}
        
        model.add(IloMaximize(masterenv, objExpr));
        objExpr.end();
    
        // Model Solution
        IloCplex cplex(masterenv);
    
        // Turn off all standard output
        cplex.setOut(masterenv.getNullStream());
        cplex.setWarning(masterenv.getNullStream());
        cplex.setError(masterenv.getNullStream());

        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::Integrality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Optimality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Threads, nThreads);
    
        cplex.extract(model);
    
        //std::ostringstream filename;
        //filename << "output\\master_0.lp";
        //cplex.exportModel(filename.str().c_str());
    
        IloBool feasible = cplex.solve();

        if (feasible == IloTrue) {
            sol.feasible = 1;
            sol.objective = cleanDouble(cplex.getObjValue());
            sol.x.resize(data.nN, std::vector<double>(data.nR, 0));
            sol.alpha.resize(data.nTotalChildren, 0);
    
            for (int i = 0; i < data.nN; i++) {
                for (int r = 0; r < data.nR; r++) {
                    if (std::abs(cplex.getValue(x[i][r]) - 1.0) < EPSILON) {
                        sol.x[i][r] = 1;
                    }
                    else {
    					sol.x[i][r] = 0;
                    }
                    
                }
            }
    
        }
        else {
            sol.feasible = 0;
        }
        
        cplex.end();
        model.end();
    
    }
    catch (IloException& e) {
        std::cerr << "CPLEX Exception: " << e.getMessage() << "\n";
    }
    
    masterenv.end();
    return sol;
}

CPLEXMasterSolution solveMaster(const MasterData& data, const bool writeLogFile, Logger& logger, const double& LB, const double& UB, const bool& wasGivenInitialSolution)
{
    // Model Definition
    IloEnv masterenv; // environment object
    IloModel model(masterenv);
    IloCplex cplex(model);

    CPLEXMasterSolution sol;

    try {
        
       //---------------------------
       //-----Decision Variables-----
       //---------------------------- 
       
        // variable x binary
        IloArray <IloBoolVarArray> x(masterenv, data.nN);
        for (int i = 0; i < data.nN; i++) {
            x[i] = IloBoolVarArray(masterenv, data.nR);
            for (int r = 0; r < data.nR; r++) {
                x[i][r] = IloBoolVar(masterenv);
                std::string varName = "x(" + std::to_string(i + 1) + "," + std::to_string(r + 1) + ")";
                x[i][r].setName(varName.c_str());
            }
        }

        //// variable u continuous
        //IloArray<IloNumVar> u(masterenv, data.nN);
        //for (int i = 0; i < data.nN; i++) {
        //    u[i] = IloNumVar(masterenv, 0, IloInfinity, ILOFLOAT);
        //    std::string varName = "u(" + std::to_string(i + 1) + ")";
        //    u[i].setName(varName.c_str());
        //}
        //
        //// variable v continuous
        //IloArray<IloNumVar> v(masterenv, data.nN);
        //for (int i = 0; i < data.nN; i++) {
        //    v[i] = IloNumVar(masterenv, 0, IloInfinity, ILOFLOAT);
        //    std::string varName = "v(" + std::to_string(i + 1) + ")";
        //    v[i].setName(varName.c_str());
        //}

        // variable alpha continuous
        IloArray<IloNumVar> alpha(masterenv, data.nTotalChildren);
        for (int t = 0; t < data.nTotalChildren; t++) {
            alpha[t] = IloNumVar(masterenv, 0, IloInfinity, ILOFLOAT);
            std::string varName = "alpha(" + std::to_string(t + 1) + ")";
            alpha[t].setName(varName.c_str());
        }
        
        //-----------------------------------
        //------------Constraints------------
        //-----------------------------------
        //Every request is allocated.
        for (int r = 0; r < data.nR; r++) {
            IloExpr sum(masterenv);
            for (int i = 0; i < data.nN; i++) {
                sum += x[i][r];
            }
            model.add(sum == 1);
            sum.end();
        }

        //Maximum Capacity - NEW CONSTRAINT
        for (int h = 0; h < data.nH; h++) {
            IloExpr cap(masterenv);
            for (int r = 0; r < data.nR; r++) {
                cap += data.q[data.nN-1][r][h] * x[data.nN - 1][r];
            }
            model.add(cap <= data.Q[data.nN - 1][h]);
            cap.end();
        }

        //Fix requests already allocated
        if (wasGivenInitialSolution) {
            for (int r = 0; r < data.nR; r++) {
                model.add(x[data.t[r]][r] == 1);
                //std::cout << "t[" << r << "] = " << data.t[r] << "\n";
            }
        }

		////Underbooking
        //for (int i = 0; i < data.nN; i++) {
        //    IloExpr under(masterenv);
        //    for (int r = 0; r < data.nR; r++) {
        //        under += data.q[i][r][0] * x[i][r];
        //    }
        //    model.add(u[i] >= data.Q[i][0] - under);
        //    under.end();
        //}
        //
        ////Overbooking
        //for (int i = 0; i < data.nN; i++) {
        //    IloExpr over(masterenv);
        //    for (int r = 0; r < data.nR; r++) {
        //        over += data.q[i][r][0] * x[i][r];
        //    }
        //    model.add(v[i] >= over - data.Q[i][0]);
        //    over.end();
        //}

        //Subproblem cuts
        int alpha_index = 0;
        for (int k = 0; k < data.nK; k++) {
            bool addCutOnce = false;
            for (int t = 0; t < data.generalisChildFeasable[k].size(); t++) {
                IloExpr cutSum(masterenv);
                for (int i = 0; i < data.nN; i++) {
                    for (int r = 0; r < data.nR; r++) {
                        if (fabs(data.generalxFound[k][i][r] - 1.0) < EPSILON)
                        {
                            cutSum += 1 - x[i][r];
                        }
                        else {
                            cutSum += x[i][r];
                        }
                    }
                }
                if (data.generalisChildFeasable[k][t] == 1) {
                    model.add(alpha[alpha_index] >= cleanDouble(data.generalzChild[k][t]) - cleanDouble(data.generalzChild[k][t] + EPSILON) * cutSum);
                    std::ostringstream oss;
                    oss << cutSum;
                    if (writeLogFile) {
                        logger.log("Optimality cut added: alpha[" + std::to_string(alpha_index + 1) + "] >= " + std::to_string(data.generalzChild[k][t]) + " - " + std::to_string(cleanDouble(data.generalzChild[k][t] + EPSILON)) + " * ( " + oss.str() + " )" + "\n");
                    }
                }
                else if (!addCutOnce) {
                    addCutOnce = true;
                    model.add(cutSum >= 1);
                    std::ostringstream oss;
                    oss << cutSum;
                    if (writeLogFile){
                        logger.log("Feasibility cut added: " + oss.str() + " >= 1\n");
                    }
                }
                cutSum.end();
                alpha_index++;
            }
        }

        //Heuristic artificial cuts
        if (!data.generalxArtificial.empty()) {
            for (int t = 0; t < data.generalxArtificial.size(); t++) {
                IloExpr artificialCutSum(masterenv);
                for (int i = 0; i < data.nN; i++) {
                    for (int r = 0; r < data.nR; r++) {
                        if (std::abs(data.generalxArtificial[t][i][r] - 1.0) < EPSILON) {
                            artificialCutSum += 1 - x[i][r];
                        }
                        else {
                            artificialCutSum += x[i][r];
                        }
                    }
                }
                model.add(artificialCutSum >= 1);
                std::ostringstream oss;
                oss << artificialCutSum;
                logger.log("Feasibility cut added: " + oss.str() + " >= 1\n");
                artificialCutSum.end();
            }
        }

        IloExpr UBobjective(masterenv);
        IloExpr LBobjective(masterenv);
        for (int i = 0; i < data.nN; i++) {
            for (int r = 0; r < data.nR; r++) {
                UBobjective += x[i][r] * data.a[i][r];
                LBobjective += x[i][r] * data.a[i][r];
            }
        }
        
        for (int t = 0; t < data.nTotalChildren; t++) {
            UBobjective += -alpha[t];
            LBobjective += -alpha[t];
        }
        
        model.add(UBobjective <= UB * (1.0 + EPSILON));
        model.add(LBobjective >= LB * (1.0 - EPSILON));
        UBobjective.end();
        LBobjective.end();

        // Objective Function
        IloExpr objExpr(masterenv);
        for (int i = 0; i < data.nN; i++) {
            for (int r = 0; r < data.nR; r++) {
                objExpr += x[i][r] * data.a[i][r];
            }
        }
        
        //----Fictional Costs----
        //for (int i = 0; i < data.nN; i++) {
        //    objExpr += - data.e * u[i] - data.b * v[i];
        //}

        for (int t = 0; t < data.nTotalChildren; t++) {
            objExpr += -alpha[t];
        }

        model.add(IloMaximize(masterenv, objExpr));
        objExpr.end();

        // Model Solution
        IloCplex cplex(masterenv);

        // Turn off all standard output
        // Turn off all standard output
        cplex.setOut(masterenv.getNullStream());
        cplex.setWarning(masterenv.getNullStream());
        cplex.setError(masterenv.getNullStream());

        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::Integrality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Optimality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Threads, nThreads);

        cplex.extract(model);
        //std::ostringstream filename;
        //filename << "output\\master_0.lp";
        //cplex.exportModel(filename.str().c_str());

        IloBool feasible = cplex.solve();        

        if (feasible == IloTrue) {
            sol.feasible = 1;
            sol.objective = cleanDouble(cplex.getObjValue());

            sol.x.resize(data.nN, std::vector<double>(data.nR, 0));
            sol.alpha.resize(data.nTotalChildren, 0);
			//sol.u.resize(data.nN, 0);
			//sol.v.resize(data.nN, 0);

            for (int i = 0; i < data.nN; i++) {
                for (int r = 0; r < data.nR; r++) {
                    if (std::abs(cplex.getValue(x[i][r]) - 1) < EPSILON) {
                        sol.x[i][r] = 1;
                    }
                    else {
                        sol.x[i][r] = 0;
                    }
                }
            }

            //std::cout << "u = [";
			//for (int i = 0; i < data.nN; i++) {
			//	std::cout << cleanDouble(cplex.getValue(u[i])) << " ";
			//}
			//std::cout << "]\n";
			//std::cout << "v = [";
			//for (int i = 0; i < data.nN; i++) {
			//	std::cout << cleanDouble(cplex.getValue(v[i])) << " ";
			//}
			//std::cout << "]\n";

            for (int t = 0; t < data.nTotalChildren; t++) {
                sol.alpha[t] = cleanDouble(cplex.getValue(alpha[t]));
            }
        }
        else {
            sol.feasible = 0;
        }

        cplex.end();
        model.end();

    }
    catch (IloException& e) {
        std::cerr << "CPLEX Exception: " << e.getMessage() << "\n";
    }

    masterenv.end();
    return sol;

}

CPLEXSubproblemSolution solveSubproblem(const SubproblemData& data, const bool writeLogFile, Logger& logger)
{
    // Model Definition
    IloEnv subenv;
    IloModel model(subenv);
    IloCplex cplex(model);

    CPLEXSubproblemSolution sol;

    try {
        int nChildren = 0;
        if (data.nK!=0) {
            nChildren = data.shared->nScenariosPerAllo[data.iStar];
        }

        //----------------------------------
        //--------Decision Variables--------
        //----------------------------------
        //IloNumVar u = IloNumVar(subenv, 0.0, IloInfinity, ILOFLOAT);
        //u.setName("u");
        //
        //IloNumVar v = IloNumVar(subenv, 0.0, IloInfinity, ILOFLOAT);
        //v.setName("v");

        IloArray<IloBoolVarArray> w(subenv, data.shared->nN);
        for (int j = 0; j < data.shared->nN; j++) {
            w[j] = IloBoolVarArray(subenv, data.shared->nR);
            for (int r = 0; r < data.shared->nR; r++) {
                w[j][r] = IloBoolVar(subenv);
                std::string varName = "w(" + std::to_string(j + 1) + "," + std::to_string(r + 1) + ")";
                w[j][r].setName(varName.c_str());
            }
        }

        IloArray <IloNumVar> alpha(subenv, nChildren);
        for (int t = 0; t < nChildren; t++) {
            alpha[t] = IloNumVar(subenv, 0.0, IloInfinity, ILOFLOAT);
            std::string varName = "alpha(" + std::to_string(t + 1) + ")";
            alpha[t].setName(varName.c_str());
        }
        //-----------------------------------
        //------------Constraints------------
        //----------------------------------- 
        
        //------------------------------------
        //-----------Force Solution-----------
        //------------------------------------
        //Checks if there is overbooking in this problem
        bool isThereReallocation = false;
        for (int t = 0; t < data.iStar - 1; t++) {
            for (int rIndex = 0; rIndex < data.shared->nR; rIndex++) {
                if (std::abs(data.wParent[t][data.iStar - 1][rIndex] - 1) < EPSILON) {
                    isThereReallocation = true;
                    break;
                }
            }
            if (isThereReallocation) {
                break;
            }
        }
        
        //Constraint makes sure that u and v are zero, if the number of containers that show up is equal to the capacity.
        if (data.shared->xi[data.iStar - 1][data.sStar - 1][0] <= data.shared->Q[data.iStar - 1][0] && not isThereReallocation) {
            for (int j = 0; j < data.shared->nN; j++) {
                for (int r = 0; r < data.shared->nR; r++) {
                    model.add(w[j][r] == 0);
                }
            }
        }
        else
        {

            //If Containers Were Allocated, Then Can Be Reallocated (Valid Inequality)
            for (int r = 0; r < data.shared->nR; r++) {
                IloExpr Const5(subenv);
                for (int j = data.iStar; j < data.shared->nN; j++) {
                    Const5 += w[j][r];
                }
                //model.add(Const5 <= data.shared->xParent[data.iStar - 1][r]);
                model.add(Const5 <= data.shared->gamma[data.iStar - 1][data.sStar - 1][r]);
                Const5.end();
            }

            for (int j = data.iStar; j < data.shared->nN; j++) {
                for (int r = 0; r < data.shared->nR; r++) {
                    model.add(data.shared->xParent[data.iStar - 1][r] * data.shared->gamma[data.iStar - 1][data.sStar - 1][r] >= w[j][r]);
                }
            }

            //Maximum Capacity Can not be exeeded
            for (int h = 0; h < data.shared->nH; h++) {
                IloExpr Cap(subenv);
                Cap += data.shared->xi[data.iStar - 1][data.sStar - 1][h];


                for (int t = 0; t < data.iStar - 1; t++) {
                    for (int r = 0; r < data.shared->nR; r++) {
                        Cap += data.shared->q[data.iStar - 1][r][h] * data.wParent[t][data.iStar - 1][r];                   
                    }
                }

                for (int j = data.iStar; j < data.shared->nN; j++) {
                    for (int r = 0; r < data.shared->nR; r++) {
                        Cap -= data.shared->q[data.iStar - 1][r][h] * w[j][r];
                    }
                }
                model.add(Cap <= data.shared->Q[data.iStar - 1][h]);
                Cap.end();
            }
            

            //Null Variables
            for (int j = 0; j <= data.iStar - 1; j++) {
                for (int r = 0; r < data.shared->nR; r++) {
                    model.add(w[j][r] == 0);
                }
            }
        }

        //-----------------------------------
       //-------------LBBD Cuts-------------
       //-----------------------------------
       //Add cuts from previous iterations
        for (int k = 0; k < data.nK; k++) {
            bool addCutOnce = false;

            for (int t = 0; t < data.generalisChildFeasable[k].size(); t++) {
                IloExpr cutSum(subenv);
                for (int r = 0; r < data.shared->nR; r++) {
                    for (int j = 0; j < data.shared->nN; j++) {
                        if (fabs(data.generalwFound[k][j][r] - 1.0) < EPSILON)
                        {
                            cutSum += 1 - w[j][r];
                        }
                        else {
                            cutSum += w[j][r];
                        }
                    }
                }

                if (data.generalisChildFeasable[k][t] == 1) {
                    model.add(alpha[t] >= cleanDouble(data.generalzChild[k][t]) - data.CutBigMVector[t] * cutSum);
                    std::ostringstream oss;
                    oss << cutSum;
                    if (writeLogFile) {
                        logger.log("Optimality cut added: alpha[" + std::to_string(t + 1) + "] >= " + std::to_string(data.generalzChild[k][t]) + " - " + std::to_string(data.CutBigMVector[t]) + " * ( " + oss.str() + " )" + "\n");
                    }
                }
                else if ((!addCutOnce)) {
                    addCutOnce = true;
                    model.add(cutSum >= 1);
                    std::ostringstream oss;
                    oss << cutSum;
                    if (writeLogFile) {
                        logger.log("Feasibility cut added: " + oss.str() + " >= 1\n");
                    }
                }
                cutSum.end();
            }
        }

        //----------------------------------
        //--------Objective Function--------
        //----------------------------------
        // Objective Function
        IloExpr objExpr(subenv);
        //objExpr += data.rho[data.iStar - 1][data.sStar - 1] * data.e * u;

        for (int j = data.iStar; j < data.shared->nN; j++) {
            for (int r = 0; r < data.shared->nR; r++) {
                //objExpr += data.rho[data.iStar - 1][data.sStar - 1] * (data.c[data.iStar - 1][j][r] + data.b) * w[j][r];
                objExpr += (data.shared->c[data.iStar - 1][j][r]) * w[j][r];
                //objExpr += (data.shared->rho[data.iStar - 1][data.sStar - 1] * data.shared->c[data.iStar - 1][j][r]) * w[j][r];
            }
        }

        objExpr = objExpr * data.shared->rho[data.iStar - 1][data.sStar - 1];

        for (int t = 0; t < nChildren; t++) {
            objExpr += alpha[t];
        }

        // Set the objective
        model.add(IloMinimize(subenv, objExpr));
        objExpr.end(); 

        // Model Solution
        IloCplex cplex(subenv);

        // Turn off all standard output
        cplex.setOut(subenv.getNullStream());
        cplex.setWarning(subenv.getNullStream());
        cplex.setError(subenv.getNullStream());

        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::Integrality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Optimality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Threads, 1);

        cplex.extract(model);
        
        //std::ostringstream filename;
        //filename << "output\\subproblem_" << data.iStar << "_" << data.sStar << ".lp";
        //std::ifstream infile(filename.str());
        //cplex.exportModel(filename.str().c_str());
       

        IloBool feasible = cplex.solve();

        if (feasible == IloTrue) {
            sol.feasible = 1;
            sol.objective = cleanDouble(cplex.getObjValue());
            //sol.objective = cleanDouble(cplex.getObjValue() * data.shared->rho[data.iStar - 1][data.sStar - 1]);

            // Extract w
            sol.w.resize(data.shared->nN);
            for (int j = 0; j < data.shared->nN; j++) {
                sol.w[j].resize(data.shared->nR);
                for (int r = 0; r < data.shared->nR; r++) {
                    if (std::abs(cplex.getValue(w[j][r]) - 1) < EPSILON) {
                        sol.w[j][r] = 1;
                    }
                    else {
                        sol.w[j][r] = 0;
                    }
                }
            }

			// Extract alpha
			sol.alpha.resize(nChildren, 0.0);
            for (int t = 0; t < nChildren; t++) {
                sol.alpha[t] = cleanDouble(cplex.getValue(alpha[t]));
            }

        }
        else {
            sol.feasible = 0;
        }

        
        cplex.end();
        model.end();
    }
    catch (IloException& e) {
        std::cerr << "CPLEX Exception: " << e.getMessage() << "\n";
    }

    subenv.end();
    return sol;
}

void writeHardMasterSolution(const CPLEXMasterSolution& sol, const MasterData& data, const std::string& instanceName, const std::chrono::steady_clock::time_point start)
{
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::filesystem::path dir = std::filesystem::path("output") / instanceName;
    std::filesystem::create_directories(dir);
    std::filesystem::path filename = dir / ("SolutionHardCapacity_" + instanceName + ".txt");
    std::ofstream file(filename);

    file << "HARD CAPACITY CONSTRAINTS VERSION" << std::endl;
    file << "STATUS: ";
    if (sol.feasible == 1)
    {
        file << "Problem feasible." << std::endl;
        file << "OPTIMIZATION TIME: " << elapsed.count() / 1000.0 << " s\n\n";

        file << "REVENUE: " << sol.objective << "\n\n";

        for (size_t i = 0; i < sol.x.size(); i++) {
            int totalBooking = 0;
            file << "Service " << i + 1 << ": ";
            for (size_t r = 0; r < sol.x[i].size(); r++) {
                int binaryVal = static_cast<int>(std::round(sol.x[i][r]));
                if (binaryVal == 1) {
                    file << r + 1 << ", ";
                    totalBooking += data.q[i][r][0];
                }
            }
            file << "\t\t" << totalBooking << "/" << data.Q[i][0] << "\n";

        }     
        file << "\n\n";
        file << "Z* = " << sol.objective << std::endl; // objective
		file << "X* = [" << "\n\t";
        for (size_t i = 0; i < sol.x.size(); i++) {
            for (size_t r = 0; r < sol.x[i].size(); r++) {
                if (std::abs(sol.x[i][r] - 1) < EPSILON) {
					file << "1 ";
                }
                else {
					file << "0 ";
                }
            }
            file << "\n\t";
        }
		file << "]\n" << std::endl;
    }
    else {
        file << "\nProblem infeasible." << std::endl;
    }
    file.close();
}

void logMasterSolution(const CPLEXMasterSolution& sol, Logger& logger) {

    if (sol.feasible == 1) {
        logger.log("\nSolution: X = [ [ ");
        for (size_t i = 0; i < sol.x.size(); i++) {
            for (size_t r = 0; r < sol.x[i].size(); r++) {
                if (std::abs(sol.x[i][r] - 1) < EPSILON) {
                    logger.log("1 ");
                }
                else {
                    logger.log("0 ");
                }
            }
            if (i != sol.x.size() - 1) {
                logger.log("] [ ");
            }
            else {
                logger.log("] ");
            }
        }
        logger.log("]");

        for (size_t t = 0; t < sol.alpha.size(); t++) {
            if (sol.alpha[t] > EPSILON) {
                logger.log("\t\tAlpha[" + std::to_string(t + 1) + "] = " + std::to_string(sol.alpha[t]) + "\t");
            }

        }
    }
    logger.log("\n\n");
}

void logSubproblemSolution(const CPLEXSubproblemSolution& sol, Logger& logger) {

    if (sol.feasible == 1) {
        logger.log("\nSolution: W = [ [ ");
        for (size_t j = 0; j < sol.w.size(); j++) {
            for (size_t r = 0; r < sol.w[j].size(); r++) {
                if (std::abs(sol.w[j][r] - 1) < EPSILON) {
                    logger.log("1 ");
                }
                else {
                    logger.log("0 ");
                }
            }
            if (j != sol.w.size() - 1) {
                logger.log("] [ ");
            }
            else {
                logger.log("] ");
            }
        }
        logger.log("]");
        
        for (size_t t = 0; t < sol.alpha.size(); t++) {
            if (sol.alpha[t] > EPSILON) {
                logger.log("\t\tAlpha[" + std::to_string(t + 1) + "] = " + std::to_string(sol.alpha[t]) + "\t");
            }
        
        }
    }
    logger.log("\n\n");
}

void logSubproblemImportedVariables(const SubproblemData& data, Logger& logger) {
    logger.log("Imported Variables: X = [ [");
    for (size_t i = 0; i < data.shared->xParent.size(); i++) {
        for (size_t r = 0; r < data.shared->xParent[i].size(); r++) {
            if (std::abs(data.shared->xParent[i][r] - 1) < EPSILON) {
                logger.log("1 ");
            }
            else {
                logger.log("0 ");
            }
        }
        if (i != data.shared->xParent.size() - 1) {
            logger.log("] [ ");
        }
        else {
            logger.log("] ");
        }
    }
    logger.log("]");

    if (!data.wParent.empty()) {
        for (size_t t = 0; t < data.wParent.size(); t++) {
			logger.log("\t\tW[" + std::to_string(t + 1) + "jr] = [ [ ");
            for (size_t j = 0; j < data.wParent[t].size(); j++) {
                for (size_t r = 0; r < data.wParent[t][j].size(); r++) {
                    if (std::abs(data.wParent[t][j][r] - 1) < EPSILON) {
                        logger.log("1 ");
                    }
                    else {
                        logger.log("0 ");
                    }
                }
                if (j != data.wParent[t].size() - 1) {
                    logger.log("] [ ");
                }
                else {
                    logger.log("] ");
                }
            }
            logger.log("]");
        }
	}

	logger.log("\n");
}

double solveSubproblemTreeLB(const ProblemData& data, const std::vector<std::vector<double>> x)
{
    std::vector<double> rho(data.nN, 1.0);
    std::vector<std::vector<double>> xi(data.nN, std::vector<double>(data.nH, 0.0));
    std::vector<std::vector<double>> gamma = x;

    std::vector<double> rhoLastScenario(data.nN, 1.0);

    for (int i = 0; i < data.nN; i++) {
        // Compute rho_i
        double prod = 1.0;
        double prodLastScenario = 1.0;
        for (int r = 0; r < data.nR; ++r) {
            if (x[i][r] == 1) {
                prod *= data.p[r];
                prodLastScenario *= (1 - data.p[r]);
            }
        }

        if (i == 0) {
            rho[i] = prod;
            rhoLastScenario[i] = prodLastScenario;
        }
        else {
            rho[i] = rho[i - 1] * prod;
            rhoLastScenario[i] = rhoLastScenario[i - 1] * prodLastScenario;
        }

        // Compute xi_{ih}
        for (int h = 0; h < data.nH; h++) {
            double sum = 0.0;
            for (int r = 0; r < data.nR; r++) {
                sum += data.q[i][r][h] * x[i][r];
            }
            xi[i][h] = sum;
        }

    }

    // Model Definition
    IloEnv subLBEnv;
    IloModel model(subLBEnv);
    IloCplex cplex(model);

    double subproblemTreeLB = -INFINITY;
    try {
        //-----------------------------------
        //---------------Big M---------------
        //-----------------------------------
        //std::vector<double> constraintBigM3(data.nN);
        //for (int i = 0; i < data.nN; i++) {
        //    double sumXq = 0.0;
        //    for (int r = 0; r < data.nR; r++) {
        //        if (std::abs(x[i][r] - 1) < EPSILON) {
        //            sumXq += data.q[i][r][0];
        //        }
        //    }
        //
        //    if (xi[i][0] <= data.Q[i][0]) {
        //        constraintBigM3[i] = 0.0;
        //    }
        //    else {
        //        constraintBigM3[i] = sumXq / (xi[i][0] - data.Q[i][0]);
        //    }
        //}

        //----------------------------------
        //--------Decision Variables--------
        //----------------------------------

        //IloArray<IloNumVar> u(subLBEnv, data.nN);
        //for (int i = 0; i < data.nN; i++) {
        //    u[i] = IloNumVar(subLBEnv, 0.0, IloInfinity, ILOFLOAT);
        //    std::string varName = "u(" + std::to_string(i + 1) + ")";
        //    u[i].setName(varName.c_str());
        //}
        //
        //IloArray<IloNumVar> v(subLBEnv, data.nN);
        //for (int i = 0; i < data.nN; i++) {
        //    v[i] = IloNumVar(subLBEnv, 0.0, IloInfinity, ILOFLOAT);
        //    std::string varName = "v(" + std::to_string(i + 1) + ")";
        //    v[i].setName(varName.c_str());
        //}

        IloArray<IloArray<IloBoolVarArray>> w(subLBEnv, data.nN);
        for (int i = 0; i < data.nN; i++) {
            w[i] = IloArray<IloBoolVarArray>(subLBEnv, data.nN);
            for (int j = 0; j < data.nN; j++) {
                w[i][j] = IloBoolVarArray(subLBEnv, data.nR);
                for (int r = 0; r < data.nR; r++) {
                    w[i][j][r] = IloBoolVar(subLBEnv);
                    std::string varName = "w(" + std::to_string(i + 1) + "," + std::to_string(j + 1) + "," + std::to_string(r + 1) + ")";
                    w[i][j][r].setName(varName.c_str());
                }
            }
        }
        //-----------------------------------
        //------------Constraints------------
        //----------------------------------- 

        ////Underbooked Containers
        //for (int i = 0; i < data.nN; i++) {
        //    IloExpr Const1(subLBEnv);
        //    Const1 += data.Q[i][0] - xi[i][0];
        //    for (int t = 0; t < i; t++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            Const1 -= data.q[i][r][0] * w[t][i][r];
        //        }
        //    }
        //    for (int j = i + 1; j < data.nN; j++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            Const1 += data.q[i][r][0] * w[i][j][r];
        //        }
        //    }
        //    model.add(u[i] >= Const1);
        //    Const1.end();
        //}
        //
        ////Overerbooked Containers
        //for (int i = 0; i < data.nN; i++) {
        //    IloExpr Const2(subLBEnv);
        //    Const2 += xi[i][0] - data.Q[0][0];
        //    for (int t = 0; t < i; t++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            Const2 -= data.q[i][r][0] * w[t][i][r];
        //        }
        //    }
        //    model.add(v[i] >= Const2);
        //    Const2.end();
        //}
        //
        ////Reallocation Is Bigger Then Overbooking
        //for (int i = 0; i < data.nN; i++) {
        //    IloExpr Const3(subLBEnv);
        //    for (int j = i + 1; j < data.nN; j++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            Const3 += data.q[i][r][0] * w[i][j][r];
        //        }
        //    }
        //    model.add(Const3 >= v[i]);
        //    Const3.end();
        //}
        //
        ////If Overboking Is 0, Then Reallocation Is 0
        //for (int i = 0; i < data.nN; i++) {
        //    IloExpr Const4(subLBEnv);
        //    for (int j = i + 1; j < data.nN; j++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            Const4 += data.q[i][r][0] * w[i][j][r];
        //        }
        //    }
        //    model.add(Const4 <= constraintBigM3[i] * v[i]);
        //    Const4.end();
        //}
        
        //If Containers Were Allocated, Then Can Be Reallocated (Valid Inequality)
        for (int i = 0; i < data.nN; i++) {
            for (int r = 0; r < data.nR; r++) {
                IloExpr Const5(subLBEnv);
                for (int j = i + 1; j < data.nN; j++) {
                    Const5 += w[i][j][r];
                }
                model.add(Const5 <= x[i][r]);
                Const5.end();
            }
        }
        
        ////Connect Varaibles
        //for (int i = 0; i < data.nN; i++) {
        //    for (int j = i + 1; j < data.nN; j++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            model.add(x[i][r] + gamma[i][r] >= 2 * w[i][j][r]);
        //        }
        //    }
        //}
        //
        ////Cutting Planes
        //for (int i = 0; i < data.nN; i++) {
        //    for (int j = i+1; j < data.nN; j++) {
        //        for (int r = 0; r < data.nR; r++) {
        //            model.add(w[i][j][r] <= x[i][r]);
        //            model.add(w[i][j][r] <= gamma[i][r]);
        //        }
        //    }
        //}

        //NEW CONSTRAINT
        for (int i = 0; i < data.nN; i++) {
            for (int j = i + 1; j < data.nN; j++) {
                for (int r = 0; r < data.nR; r++) {
                    model.add(x[i][r] * gamma[i][r] >= w[i][j][r]);
                }
            }
        }
        
        //Maximum Capacity Can not be exeeded
        for (int i = 0; i < data.nN; i++) {
            for (int h = 0; h < data.nH; h++) {
                IloExpr Cap(subLBEnv);
                Cap += xi[i][h];
                for (int t = 0; t < i; t++) {
                    for (int r = 0; r < data.nR; r++) {
                        Cap += data.q[i][r][0] * w[t][i][r];
                    }
                }
                for (int j = i + 1; j < data.nN; j++) {
                    for (int r = 0; r < data.nR; r++) {
                        Cap -= data.q[i][r][h] * w[i][j][r];
                    }
                }
                model.add(Cap <= data.Q[i][h]);
                Cap.end();
            }
        }

        //Null Variables
        for (int i = 0; i < data.nN; i++) {
            for (int j = 0; j <= i; j++) {
                for (int r = 0; r < data.nR; r++) {
                    model.add(w[i][j][r] == 0);
                }
            }
        }

        //----------------------------------
        //--------Objective Function--------
        //----------------------------------
        // Objective Function
        IloExpr objExpr(subLBEnv);
       //for (int i = 0; i < data.nN; i++) {
       //    objExpr += rho[i] * data.e * u[i];
       //}
        for (int i = 0; i < data.nN; i++) {
            for (int j = i + 1; j < data.nN; j++) {
                for (int r = 0; r < data.nR; r++) {
                    objExpr += rho[i] * (data.c[i][j][r]) * w[i][j][r];
                    //objExpr += rho[i] * (data.c[i][j][r] + data.b) * w[i][j][r];
                }
            }

        }

        // Set the objective
        model.add(IloMinimize(subLBEnv, objExpr));
        objExpr.end();

        // Model Solution
        IloCplex cplex(subLBEnv);

        // Turn off all standard output
        cplex.setOut(subLBEnv.getNullStream());
        cplex.setWarning(subLBEnv.getNullStream());
        cplex.setError(subLBEnv.getNullStream());

        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Feasibility, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::MIP::Tolerances::Integrality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Simplex::Tolerances::Optimality, NUMERICAL_ZERO);
        cplex.setParam(IloCplex::Param::Threads, 1);

        cplex.extract(model);
        
        //std::ostringstream filename;
        //filename << "output\\subproblemTreeLB.lp";
        //std::ifstream infile(filename.str());
        //cplex.exportModel(filename.str().c_str());

        IloBool feasible = cplex.solve();

        if (feasible == IloTrue) {
            subproblemTreeLB = cleanDouble(cplex.getObjValue());

            //for (int i = 0; i < data.nN; i++) {
            //    subproblemTreeLB += cleanDouble(rhoLastScenario[i] * data.e * data.Q[i][0]);
            //}

			//std::cout << "Solution is feasible with objective value: " << subproblemTreeLB << "\n";
            //
            //
            //// Extract w
            //for (int i = 0; i < data.nN; i++) {
            //    for (int j = 0; j < data.nN; j++) {
            //        for (int r = 0; r < data.nR; r++) {
            //            if (cplex.getValue(w[i][j][r]) > 0.5) {
            //                std::cout << "w[" << i+1 << "][" << j + 1 << "][" << r + 1 << "] = 1\n";
            //                std::cout << "c = " << data.c[i][j][r] << "\n";
            //            }
            //        }
            //    }
            //}

        }

        cplex.end();
		model.end();
    }
    catch (IloException& e) {
        std::cerr << "CPLEX Exception: " << e.getMessage() << "\n";
    }

    subLBEnv.end();
    return subproblemTreeLB;
}