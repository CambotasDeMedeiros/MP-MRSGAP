#include "../include/preprocessing.h"

ProblemData readParametersFromFile(const std::filesystem::path& filename) {
    ProblemData data;
    std::ifstream fin(filename);
    if (!fin) return data;

    auto trim = [](std::string& s) {
        auto not_space = [](unsigned char c) { return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
        s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
        };

    auto parse_vec1i = [&](const std::string& v, std::vector<int>& out) {
        out.clear();
        std::string t = v.substr(1, v.size() - 2);
        std::stringstream ss(t);
        std::string tok;
        while (std::getline(ss, tok, ',')) out.push_back(std::stoi(tok));
    };

    auto parse_vec1d = [&](const std::string& v, std::vector<double>& out) {
        out.clear();
        std::string t = v.substr(1, v.size() - 2);
        std::stringstream ss(t);
        std::string tok;
        while (std::getline(ss, tok, ',')) out.push_back(std::stod(tok));
    };

    auto parse_vec2i = [&](const std::string& v, std::vector<std::vector<int>>& out) {
        out.clear();
        std::string t = v;
        int depth = 0;
        std::string buf;
        for (char c : t) {
            if (c == '[') { depth++; if (depth == 2) buf.clear(); continue; }
            if (c == ']') {
                depth--;
                if (depth == 1) {
                    std::vector<int> row;
                    std::string wrapped = "[" + buf + "]";
                    parse_vec1i(wrapped, row);
                    out.push_back(row);
                }
                continue;
            }
            if (depth == 2) buf += c;
        }
    };

    auto parse_vec3i = [&](const std::string& s, std::vector<std::vector<std::vector<int>>>& out) {
        out.clear();
        std::string t = s;
        t.erase(std::remove_if(t.begin(), t.end(), ::isspace), t.end());
        if (t.front() == '[' && t.back() == ']') t = t.substr(1, t.size() - 2);

        size_t pos = 0;
        while (pos < t.size()) {
            if (t[pos] == '[') {
                int depth = 1;
                size_t start = pos + 1;
                pos++;
                while (pos < t.size() && depth > 0) {
                    if (t[pos] == '[') depth++;
                    else if (t[pos] == ']') depth--;
                    pos++;
                }
                std::string block = t.substr(start, pos - start - 1);

                std::vector<std::vector<int>> block2D;
                size_t j = 0;
                while (j < block.size()) {
                    if (block[j] == '[') {
                        int d2 = 1;
                        size_t start2 = j + 1;
                        j++;
                        while (j < block.size() && d2 > 0) {
                            if (block[j] == '[') d2++;
                            else if (block[j] == ']') d2--;
                            j++;
                        }
                        std::string row = block.substr(start2, j - start2 - 1);
                        std::vector<int> rowVec;
                        std::stringstream ss(row);
                        std::string tok;
                        while (std::getline(ss, tok, ',')) rowVec.push_back(std::stoi(tok));
                        block2D.push_back(rowVec);
                    }
                    else j++;
                }
                out.push_back(block2D);
            }
            else pos++;
        }
        };

    std::string line;
    while (std::getline(fin, line)) {
        trim(line);
        if (line.empty()) continue;

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string var = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (!val.empty() && val.back() == ';') val.pop_back();

        trim(var);
        trim(val);

        if (var == "nN") data.nN = std::stoi(val);
        else if (var == "nR") data.nR = std::stoi(val);
        else if (var == "nH") data.nH = std::stoi(val);
        else if (var == "a") parse_vec2i(val, data.a);
        else if (var == "b") data.b = std::stoi(val);
        else if (var == "c") parse_vec3i(val, data.c);
        else if (var == "e") data.e = std::stoi(val);
        else if (var == "Q") parse_vec2i(val, data.Q);
        else if (var == "q") parse_vec3i(val, data.q);
        else if (var == "p") parse_vec1d(val, data.p);
        else if (var == "t") {
            if (val.front() == '[' && val.back() == ']') {
                val = val.substr(1, val.size() - 2);
                data.t.clear();
                std::stringstream ss(val);
                std::string tok;
                while (std::getline(ss, tok, ',')) {
                    trim(tok);
                    data.t.push_back(std::stoi(tok));
                }
            }
        }
    }
    return data;
}

MasterData convertDataFromProblemToMaster(const ProblemData& prob) {
    MasterData master;

    master.nK = 0; // Initially no cuts
    master.nN = prob.nN;
    master.nR = prob.nR;
	master.nH = prob.nH;

    master.a = prob.a;
    master.c = prob.c;
	master.b = prob.b;
	master.e = prob.e;

    master.Q = prob.Q;
    master.q = prob.q;
    master.p = prob.p;
    if (!prob.t.empty()) {
        master.t = prob.t;
    }
    else {
        master.t = std::vector<int>(prob.nR, 0);
    }

    master.generalisChildFeasable = {}; // Initially no cuts
    master.generalzChild = {}; // Initially no cuts
    master.generalxFound = {}; // Initially no cuts

    return master;
}

std::vector<std::tuple<std::vector<int>, std::vector<int>, double>> generalizedPoissonBinomialPMF(const GPBDData& gpbddata, const int nH)
{
    std::vector<double> rho;
    std::vector<std::vector<int>> xi;
    std::vector<std::vector<int>> gamma;
    size_t n = gpbddata.probs.size();
    int totalComb = 1 << n;

    std::map<double, double> gpbd;

    for (int mask = 0; mask < totalComb; ++mask) {
        std::vector<int> s(n);
        double prob = 1.0;
        std::vector<int> resources(nH, 0);

        for (int i = 0; i < n; ++i) {
            s[i] = (mask >> i) & 1;
            if (s[i] == 1) {
				//std::cout << gpbddata.probs[i] << " ";
                prob *= gpbddata.probs[i];
                for (int h = 0; h < nH; h++){
                    resources[h]+= gpbddata.weightsucess[i][h];
				}
            }
            else {
                prob *= (1.0 - gpbddata.probs[i]);
                for (int h = 0; h < nH; h++) {
                    resources[h] += gpbddata.weightnotsucess[i][h];
                }
            }
        }

		//std::cout << " = " << prob << "\n";
        rho.push_back(prob);
        xi.push_back(resources);
        gamma.push_back(s);
        
    }

    std::vector<std::tuple<std::vector<int>, std::vector<int>, double>> combined;
    for (size_t i = 0; i < xi.size(); ++i) {
        combined.emplace_back(xi[i], gamma[i], rho[i]); // vector<int> first
    }
    
    // Sort by the first element of the vector<int> in descending order
    std::sort(combined.begin(), combined.end(),
        [](const auto& a, const auto& b) {
			return std::get<0>(a)[0] > std::get<0>(b)[0]; //descending order >
        }
    );
    
    //std::vector<std::tuple<std::vector<int>, std::vector<int>, double>> pmf_sorted = combined;
    //
    //// If you need another sorted copy, sort it the same way
    //std::sort(pmf_sorted.begin(), pmf_sorted.end(),
    //    [](const auto& a, const auto& b) {
    //        return std::get<0>(a)[0] < std::get<0>(b)[0];
    //    }
    //);

   
    return combined;
}

ScenariosData getScenariosData(const GeneralGPBDData& data){
    ScenariosData scenariosdata;
	scenariosdata.nH = data.nH;
    size_t num_allocations = data.allocations.size();
    scenariosdata.rho.resize(num_allocations);
    scenariosdata.xi.resize(num_allocations);
	scenariosdata.gamma.resize(num_allocations);

    for (size_t i = 0; i < num_allocations; i++) {
        const GPBDData& gpbd = data.allocations[i];
        std::vector<std::tuple<std::vector<int>, std::vector<int>, double>> pmf = generalizedPoissonBinomialPMF(gpbd, data.nH);

        scenariosdata.rho[i].reserve(pmf.size());
        scenariosdata.xi[i].reserve(pmf.size());
		scenariosdata.gamma[i].reserve(pmf.size());

        for (const auto& [xi_vec, gamma_vec, rho_val] : pmf) {
            scenariosdata.gamma[i].push_back(gamma_vec);
            scenariosdata.xi[i].push_back(xi_vec);
            scenariosdata.rho[i].push_back(rho_val);
        }

    }



    return scenariosdata;
}

void updateSubproblemData(SharedSubproblemData& subproblem, const ScenariosData& scenarioDataAllo, const ScenariosData& scenarioDataLvl, const std::vector<std::vector<double>> x)
{
	subproblem.gamma = scenarioDataLvl.gamma;
    subproblem.rho = scenarioDataLvl.rho;
    subproblem.xi = scenarioDataLvl.xi;
    subproblem.xParent = x;
    //subproblem.nS = static_cast<int>(scenarioData.xi[0].size());
    subproblem.nScenariosPerAllo.clear();
    for (const auto& xi_vec : scenarioDataAllo.xi) {
        subproblem.nScenariosPerAllo.push_back(static_cast<int>(xi_vec.size()));
	}
    subproblem.nScenariosPerLvl.clear();
    for (const auto& xi_vec : scenarioDataLvl.xi) {
        subproblem.nScenariosPerLvl.push_back(static_cast<int>(xi_vec.size()));
    }

}

SharedSubproblemData convertDataFromProblemToSubproblem(const ProblemData& prob, const std::vector <std::vector<double>> x) {
    SharedSubproblemData subproblem;
    //subproblem.nK = 0;
    subproblem.nN = prob.nN;
    subproblem.nR = prob.nR;
	subproblem.nH = prob.nH;

    subproblem.a = prob.a;
    subproblem.c = prob.c;
	subproblem.b = prob.b;
	subproblem.e = prob.e;

    subproblem.Q = prob.Q;
    subproblem.q = prob.q;
    subproblem.p = prob.p;

    subproblem.xParent;
    subproblem.rho;
    subproblem.xi;
    subproblem.gamma;

    return subproblem;

}

void printElapsedTime(const std::chrono::steady_clock::time_point start,
    const std::string& message) {
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "\n" << message << ": "
        << elapsed.count() / 1000.0 << " s\n"; // print seconds
}

GeneralGPBDData convertDataToGPBD(const std::vector <double> p, const std::vector <std::vector<std::vector<int>>> q, const std::vector<std::vector<double>> x, const int nH)
{
    GeneralGPBDData generalGPBDdata;

    for (int i = 0; i < x.size(); i++) {
        GPBDData gpbddata;
        for (int r = 0; r < x[i].size(); r++) {
            if (std::abs(x[i][r] - 1) < EPSILON) {
                gpbddata.probs.push_back(p[r]);
                gpbddata.weightsucess.push_back(q[i][r]);
                gpbddata.weightnotsucess.push_back(std::vector<int>(nH,0));
            }
        }
        generalGPBDdata.allocations.push_back(gpbddata);
    }

    generalGPBDdata.nH = nH;

    return generalGPBDdata;
}

ScenariosData getBranchScenariosData(const ScenariosData& input) {
    ScenariosData output;

    output.xi.push_back(input.xi[0]);
    output.rho.push_back(input.rho[0]);
	output.gamma.push_back(input.gamma[0]);

    for (size_t level = 1; level < input.xi.size(); ++level) {
        std::vector<std::vector<int>> newXi;
        std::vector<double> newRho;
		std::vector<std::vector<int>> newGamma;

        const auto& prevRho = output.rho.back();
        const auto& currXi = input.xi[level];
        const auto& currRho = input.rho[level];
		const auto& currGamma = input.gamma[level];

        for (size_t i = 0; i < prevRho.size(); ++i) {
            for (size_t j = 0; j < currXi.size(); ++j) {
                newXi.push_back(currXi[j]);
                //newRho.push_back(currRho[j]);
                newRho.push_back(prevRho[i] * currRho[j]);
				newGamma.push_back(currGamma[j]);
            }
        }

        output.xi.push_back(newXi);
        output.rho.push_back(newRho);
		output.gamma.push_back(newGamma);
    }

    return output;

}

void printGPBDData(const GPBDData& data, int index) {
    std::cout << "  Allocation [" << index << "]:\n";

    std::cout << "    Probs: ";
    for (size_t i = 0; i < data.probs.size(); ++i) {
        std::cout << data.probs[i];
        if (i + 1 < data.probs.size()) std::cout << ", ";
    }
    std::cout << "\n";

    std::cout << "    Weight Success:\n";
    for (size_t i = 0; i < data.weightsucess.size(); ++i) {
        std::cout << "      [" << i << "]: ";
        for (size_t j = 0; j < data.weightsucess[i].size(); ++j) {
            std::cout << data.weightsucess[i][j];
            if (j + 1 < data.weightsucess[i].size()) std::cout << ", ";
        }
        std::cout << "\n";
    }

    std::cout << "    Weight Not Success:\n";
    for (size_t i = 0; i < data.weightnotsucess.size(); ++i) {
        std::cout << "      [" << i << "]: ";
        for (size_t j = 0; j < data.weightnotsucess[i].size(); ++j) {
            std::cout << data.weightnotsucess[i][j];
            if (j + 1 < data.weightnotsucess[i].size()) std::cout << ", ";
        }
        std::cout << "\n";
    }
}

//void printGeneralGPBDData(const GeneralGPBDData& data) {
//    std::cout << "=== GeneralGPBDData ===\n";
//    std::cout << "nH: " << data.nH << "\n";
//    std::cout << "Allocations (" << data.allocations.size() << "):\n";
//    for (size_t i = 0; i < data.allocations.size(); ++i) {
//        printGPBDData(data.allocations[i], i);
//    }
//    std::cout << "======================\n";
//}

SharedSubproblemData preprocessSubproblemData(const ProblemData& problem, const std::vector<std::vector<double>> x)
{
    //std::vector <std::vector<int>> vectorQ = problem.Q;


    SharedSubproblemData subproblemdata;
    subproblemdata = convertDataFromProblemToSubproblem(problem, x);

    //printSubproblemData(subproblemdata);

    GeneralGPBDData generalgpbddata;
    generalgpbddata = convertDataToGPBD(problem.p, problem.q, x, subproblemdata.nH);


    //printGeneralGPBDData(generalgpbddata);

    ScenariosData scenariosDataAllo = getScenariosData(generalgpbddata); 

    //for (size_t t = 0; t < scenariosDataAllo.xi.size(); ++t) {
    //    std::cout << "Level " << t + 1 << ":\n";
    //    for (size_t i = 0; i < scenariosDataAllo.rho[t].size(); ++i) {
    //        // Combine xi, gamma, and rho into tuples
    //        std::cout << i << " - xi: [ ";
    //        for (const auto& val : scenariosDataAllo.xi[t][i]) {
    //            std::cout << val << " ";
    //        }
    //        std::cout << "], rho: " << scenariosDataAllo.rho[t][i] << " gamma = ";
    //        for (const auto& val : scenariosDataAllo.gamma[t][i]) {
    //            std::cout << val << " ";
    //        }
    //        std::cout << "\n";
    //    }
    //}

    //printScenariosData(scenariosDataAllo);

    std::vector<std::vector<std::vector<int>>> newGamma;
	newGamma.reserve(x.size());
    for (int i = 0; i < x.size(); i++) {
		std::vector<std::vector<int>> tempGamma;
        std::vector<int> ones_positions;
        for (int r = 0; r < x[i].size(); r++) {
            if (std::abs(x[i][r] - 1) < EPSILON) {
                ones_positions.push_back(r);
            }
        }
        for (const auto& g : scenariosDataAllo.gamma[i]) {
            std::vector<int> copyX;
            copyX.reserve(x[i].size());
            for (double val : x[i]) {
                if (std::abs(val - 1) < EPSILON) {
                    copyX.push_back(1);
                }else {
                    copyX.push_back(0);
                }
            }
            for (int j = 0; j < ones_positions.size(); j++) {
                copyX[ones_positions[j]] = g[j];
            }
            tempGamma.push_back(copyX);
        }
		newGamma.push_back(tempGamma);
    }
	scenariosDataAllo.gamma = newGamma;

    //printScenariosData(scenariosDataAllo);

    ScenariosData scenariosDataLvl = getBranchScenariosData(scenariosDataAllo);
    
    //for (size_t t = 0; t < scenariosDataLvl.xi.size(); ++t) {
    //    std::cout << "Level " << t + 1 << ":\n";
    //    for (size_t i = 0; i < scenariosDataLvl.rho[t].size(); ++i) {
    //        // Combine xi, gamma, and rho into tuples
    //        std::cout << i <<  " - xi: [ ";
    //        for (const auto& val : scenariosDataLvl.xi[t][i]) {
    //            std::cout << val << " ";
    //        }
    //        std::cout << "], rho: " << scenariosDataLvl.rho[t][i] << " gamma = ";
    //        for (const auto& val : scenariosDataLvl.gamma[t][i]) {
    //            std::cout << val << " ";
    //        }
    //        std::cout << "\n";
    //    }
    //}

    //printScenariosData(scenariosDataLvl);
    //for (auto& vec : scenariosDataLvl.rho) {        
	//	//Remove from the tree the scenarios with zero probability (It breaks the code)
    //    vec.erase(
    //        std::remove_if(vec.begin(), vec.end(),
    //           [](double x) { return x == 0.0; }),
    //       vec.end());
    //}

    //printScenariosData(scenariosDataLvl);

    updateSubproblemData(subproblemdata, scenariosDataAllo, scenariosDataLvl, x);


    return subproblemdata;
}

void writeFinalSolution(const std::vector<std::unique_ptr<Problem>>& problemTree, const std::chrono::steady_clock::time_point start, const bool stopOptimization, const double optGap, const std::string instanceName, const int iteration, const int bestIteration, const int prunedSolutions) {

    std::filesystem::path dir = std::filesystem::path("output") / instanceName;
    std::filesystem::create_directories(dir);
    std::filesystem::path filename = dir / ("FinalSolution_" + instanceName + ".txt");
    std::ofstream file(filename);
    file << std::fixed << std::setprecision(WRITING_PRECISION);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    if (stopOptimization) {
        file << "BEST SOLUTION\nOptimization Time: " << elapsed.count() / 1000.0 << " s\n";
        file << "OPTIMALITY GAP: " << optGap << "%\n";
    }
    else {
        file << "OPTIMAL SOLUTION\nOptimization Time: " << elapsed.count() / 1000.0 << " s\n";
    }
    file << "N ITERATIONS = " << iteration + 1 << "\n";

    if (problemTree.empty()) {
        file << "No solution found.\n";
        file.close();
        return;
	}

    file << "BEST ITERATION = " << bestIteration + 1 << "\n";
    file << "PRUNED SOLUTIONS = " << prunedSolutions << "\n\n";
    file << "MASTER PROBLEM" << std::endl;
    file << "Z* = ";

    for (size_t i = 0; i < problemTree.size(); i++) {
        Problem* ProblemPtr = problemTree[i].get();
        if (i == 0) {
            MasterProblem* masterProblemPtr = dynamic_cast<MasterProblem*>(ProblemPtr);
            auto masterProblemSolution = masterProblemPtr->masterSol;
            file << masterProblemSolution.objective << std::endl;

            file << "X* = [\n\t";
            for (auto row : masterProblemSolution.x) {
                for (auto value : row) {
                    if (std::abs(value - 1) < EPSILON) {
                        value = 1;
                    }
                    else {
                        value = 0;
                    }
					file << value << " ";
                }
                file << "\n\t";
            }
            file << "]\n\n";

        }
        else {
            auto SubproblemPtr = dynamic_cast<Subproblem*>(ProblemPtr);
            auto subproblemSolution = SubproblemPtr->subproblemSol;
            auto subproblemData = SubproblemPtr->subproblemData;

            file << "SUBPROBLEM(i = " << subproblemData.iStar
                << ", s = " << subproblemData.sStar << ")\n";
            file << "Z* = " << subproblemSolution.objective << "\n";
            file << "W* = [\n\t";

            for (auto& row : subproblemSolution.w) {
                for (auto& value : row) {
                    if (std::abs(value - 1) < EPSILON) {
                        value  = 1;
                    }
                    else {
                        value = 0;
                    }
                    file << value << " ";
                }
                file << "\n\t";
            }
            file << "]\n\n";
        }

    }


    file.close();
}

void writeBounds(const std::vector<double> upperBounds, const std::vector<double> lowerBounds, const std::string instanceName) {
    std::filesystem::path dir = std::filesystem::path("output") / instanceName;
    std::filesystem::create_directories(dir);
    std::filesystem::path filename = dir / ("Bounds_" + instanceName + ".txt");
    std::ofstream file(filename);

    for (int i = 0; i < upperBounds.size(); i++) {
        file << upperBounds[i] << " ";
    }
    file << "\n";
    for (int i = 0; i < lowerBounds.size(); i++) {
        file << lowerBounds[i] << " ";
    }

    file.close();
}

void writeSolutionSummary(const std::vector<std::unique_ptr<Problem>>& problemTree, const std::string instanceName) {

    std::filesystem::path dir = std::filesystem::path("output") / instanceName;
    std::filesystem::create_directories(dir);
    std::filesystem::path filename = dir / ("SolutionSummary_" + instanceName + ".txt");
    std::ofstream file(filename);
    file << std::fixed << std::setprecision(5);

    file << "TOTAL REVENUE: ";

    int nN = 0;
    int nM = 0;
    int nR = 0;

    Problem* ProblemPtr = problemTree[0].get();
    MasterProblem* masterProblemPtr = dynamic_cast<MasterProblem*>(ProblemPtr);
    auto masterProblemSolution = masterProblemPtr->masterSol;
    double revenue = masterProblemSolution.objective;

    for (size_t t = 1; t < problemTree.size(); t++) {
        Problem* ProblemPtr = problemTree[t].get();
        auto* SubproblemPtr = dynamic_cast<Subproblem*>(ProblemPtr);
        auto subproblemSolution = SubproblemPtr->subproblemSol;
        auto subproblemData = SubproblemPtr->subproblemData;
        if (subproblemData.iStar == 1) {
            revenue -= subproblemSolution.objective;
        }
        else {
            break;
        }
        nN = subproblemData.shared->nN;
        nR = subproblemData.shared->nR;
    }

    file << revenue << std::endl;

    for (size_t i = 0; i < masterProblemPtr->masterData.nN; i++) {
		int totalBooking = 0;
        file << "Service " << i+1 << ": ";
        for (size_t r = 0; r < masterProblemPtr->masterData.nR; r++) {
            int binaryVal = static_cast<int>(std::round(masterProblemSolution.x[i][r]));
            if (binaryVal == 1) {
                file << r + 1 << ", ";
                totalBooking += masterProblemPtr->masterData.q[i][r][0];
            }
        }
		file << "\t\t" << totalBooking << "/" << masterProblemPtr->masterData.Q[i][0] << "\n";
    }

    int totalBooking = 0;
    file << "\n\n";

    if (problemTree.size() > 1) {
        file << "REQUEST\t\tALLOCATION\tRE-ALLOCAATION\tSCENARIO\n";

        //int s = 0;
        for (int t = 0; t < problemTree.size(); t++) {
            auto* problemPtr = problemTree[t].get();
            std::string id = problemPtr->problemID;
            size_t pos = id.find('_');
            int firstNumber = std::stoi(id.substr(0, pos));
            //int s = std::stoi(id.substr(pos + 1, id.length() - pos - 1));
            if (firstNumber == nN) {
                int s = std::stoi(id.substr(pos + 1, id.length() - pos - 1));
                auto* subprbolemPtr = dynamic_cast<Subproblem*>(problemPtr);
                auto subproblemData = subprbolemPtr->subproblemData;
                for (int i = 0; i < nN - 1; i++) {
                    for (int j = 0; j < nN; j++) {
                        for (int r = 0; r < nR; r++) {
                            double val = subproblemData.wParent[i][j][r];
                            int binaryVal = (std::abs(val - 1) < EPSILON) ? 1 : 0;
                            if (binaryVal == 1) {
                                file << (r + 1) << "\t\t" << (i + 1) << "\t\t" << (j + 1) << "\t\t" << s << "\n";
                            }
                        }
                    }
                }

                auto subproblemSolution = subprbolemPtr->subproblemSol;
                for (int j = 0; j < nM; j++) {
                    for (int r = 0; r < nR; r++) {
                        double val = subproblemSolution.w[j][r];
                        int binaryVal = (std::abs(val - 1) < EPSILON) ? 1 : 0;
                        if (binaryVal == 1) {
                            file << (r + 1) << "\t\t" << (nN) << "\t\t" << (j + 1) << "\t\t" << s << "\n";
                        }
                    }
                }

            }

        }

        file << "\n\n";

        file << "SCENARIOS DETAILS\n";
        auto* problemPtr = problemTree.back().get();
        auto* subproblemPtr = dynamic_cast<Subproblem*>(problemPtr);
        SubproblemData subproblemData = subproblemPtr->subproblemData;
        //std::vector<std::vector<double>> levels = {};
        std::vector<std::vector<std::vector<int>>> levels = {};
        for (int i = 0; i < subproblemData.shared->nN; ++i) {
            int nScenarios = subproblemData.shared->nScenariosPerAllo[i];
            //std::vector<double> level;
            std::vector<std::vector<int>> level;

            // Copy the first nScenarios values from xi[i]
            for (int j = 0; j < nScenarios && j < static_cast<int>(subproblemData.shared->xi[i].size()); ++j) {
                //level.push_back(subproblemData.xi[i][j]);
				level.push_back(subproblemData.shared->gamma[i][j]);
            }

            levels.push_back(std::move(level));
        }
        //std::vector<std::vector<double>> result;
        //std::vector<std::vector<double>> result;
		std::vector<std::vector<std::vector<int>>> result;
        std::vector<size_t> indices(levels.size(), 0);
        while (true) {
            // Build one combination
            //std::vector<double> combination;
			std::vector<std::vector<int>> combination;
            for (size_t i = 0; i < levels.size(); ++i)
                combination.push_back(levels[i][indices[i]]);
            result.push_back(combination);

            // Increment indices
            size_t k = levels.size();
            while (k > 0) {
                --k;
                if (++indices[k] < levels[k].size())
                    break;
                indices[k] = 0;
            }
            if (k == 0 && indices[0] == 0)
                break;  // finished
        }

        for (int s = 0; s < result.size(); s++) {
            file << "Scenario " << s + 1 << ": \n";
            for (int t = 0; t < result[s].size(); t++) {
                //file << "\tTransport " << t + 1 << ": " result[s][t] << "\n";
                file << "\tTransport " << t + 1 << ": "; 
                for (int r = 0; r < result[s][t].size(); r++) {
                    if (std::abs(result[s][t][r] - 1) < EPSILON) {
                        file << r+1 << " ";
                    }
				}
                file << "\n";
            }
            file << "\n";
        }
    }

    file.close();
}