#pragma once
#include <ilcplex/ilocplex.h>
#include <filesystem>
#include <vector>
#include <iostream>
#include <string>
constexpr double EPSILON = 1e-5;
const double NUMERICAL_ZERO = 1e-6;
const int WRITING_PRECISION = 5;

struct ProblemData {
	int nN=0, nR= 0, nH= 0, e=0, b=0;
	std::vector<std::vector<int>> a = {};
	std::vector<std::vector<std::vector<int>>> c = {};
	std::vector<std::vector<int>> Q = {};
	std::vector<std::vector<std::vector<int>>> q = {};
	std::vector<double> p = {};
	std::vector<int> t = {};
};

struct MasterData {
	//Original data from the problem
	int nK, nN, nR, nH, e, b;
	std::vector<std::vector<int>> a;
	std::vector<std::vector<std::vector<int>>> c;
	std::vector<std::vector<int>> Q;
	std::vector<std::vector<std::vector<int>>> q;
	std::vector<double> p;
	std::vector<int> t;

	int nChildren = 0;
	int nTotalChildren = 0;

	//Received data from subproblems
	double CutBigM = 0;
	std::vector<bool> isChildFeasable = {};
	std::vector<double> zChild = {};
	std::vector<std::vector<double>> xFound = {};
	std::vector <std::vector<std::vector<double>>> uFound = {};
	std::vector <std::vector<std::vector<double>>> vFound = {};
	std::vector <std::vector<std::vector<std::vector<std::vector<double>>>>> wFound = {};
	std::vector <std::vector<std::vector<std::vector<std::vector<double>>>>> zFound = {};

	std::vector <double> generalCutBigM = {};
	std::vector <std::vector<bool>> generalisChildFeasable = {};
	std::vector <std::vector<double>> generalzChild = {};
	std::vector <std::vector<std::vector<double>>> generalxFound = {};
	std::vector < std::vector <std::vector<std::vector<double>>>> generaluFound = {};
	std::vector < std::vector <std::vector<std::vector<double>>>> generalvFound = {};
	std::vector < std::vector <std::vector<std::vector<std::vector<std::vector<double>>>>>> generalwFound = {};
	std::vector < std::vector <std::vector<std::vector<std::vector<std::vector<double>>>>>> generalzFound = {};

	std::vector<std::vector<std::vector<double>>> generalxArtificial = {};
};

struct SharedSubproblemData {
	std::vector<std::vector<double>> rho;
	std::vector<std::vector<std::vector<int>>> xi;
	std::vector<std::vector<std::vector<int>>> gamma;
	int nR, nN, nH, e, b;
	std::vector<int> nScenariosPerAllo, nScenariosPerLvl;
	std::vector<std::vector<int>> a;
	std::vector<std::vector<std::vector<int>>> c;
	std::vector<std::vector<int>> Q;
	std::vector<std::vector<std::vector<int>>> q;
	std::vector<double> p;
	std::vector<std::vector<double>> xParent = {};
};

struct SubproblemData {
	//Original data from the problem
	std::shared_ptr<const SharedSubproblemData> shared;
	int nS;
	int nK = 0;
	//int nR, nN, nS, nK, nH, e, b;
	//std::vector<int> nScenariosPerAllo, nScenariosPerLvl;
	//std::vector<std::vector<int>> a;
	//std::vector<std::vector<std::vector<int>>> c;
	//std::vector<std::vector<int>> Q;
	//std::vector<std::vector<std::vector<int>>> q;
	//std::vector<double> p;
	
	int iStar, sStar;

	//Received data from the master problem
	//std::vector<std::vector<double>> rho = {};
	//std::vector<std::vector<std::vector<int>>> xi = {};
	//std::vector<std::vector<std::vector<int>>> gamma = {};
	//std::vector<std::vector<double>> xParent = {};
	double uParent = {};
	double vParent = {};
	std::vector <std::vector<std::vector<double>>> wParent;
	std::vector <std::vector<std::vector<double>>> zParent;


	//Received data from previous subproblems
	double CutBigM = 0;
	std::vector<double> CutBigMVector = {};
	std::vector<bool> isChildFeasable = {};
	std::vector<double> zChild = {};
	std::vector<double> xFound = {};
	std::vector<double> uFound = {};
	std::vector<double> vFound = {};
	std::vector<std::vector<double>> wFound = {};
	std::vector<std::vector<double>> zFound = {};


	std::vector <double> generalCutBigM = {};
	std::vector <std::vector<bool>> generalisChildFeasable = {};
	std::vector <std::vector<double>> generalzChild = {};
	std::vector < std::vector <double>> generalxFound = {};
	std::vector < std::vector <double>> generaluFound = {};
	std::vector < std::vector <double>> generalvFound = {};
	std::vector <std::vector<std::vector<double>>> generalwFound = {};
	std::vector <std::vector<std::vector<double>>> generalzFound = {};

};

struct CPLEXMasterSolution {
	double objective;
	std::vector<std::vector<double>> x;
	std::vector<double> alpha;
	int feasible;
};

struct CPLEXSubproblemSolution {
	double objective;
	std::vector<std::vector<double>> w;
	std::vector<double> alpha;
	int feasible = -1;
};

class Problem {
public:
	std::string problemID;
	bool isProblemDifferent = 1;
	Problem* parentProblem;
	std::vector<Problem*> childrenProblems;
	int level;
	int scenario;
	double problemLowerBound = -INFINITY;
	double problemUpperBound = +INFINITY;

	//Constructor
	Problem(const std::string& id) : problemID(id), level(0), scenario(0) {}
	virtual ~Problem() = default;
	virtual std::unique_ptr<Problem> clone() const = 0;

	//Methods
	std::string getProblemID() const;
	void setParentProblem(Problem*& parent);
	void addChildProblem(Problem*& child);
	Problem* getParentProblem();
	std::vector<Problem*> getChildrenProblems();
	void setLevel(int l);
	void setScenario(int s);

};

class MasterProblem : public Problem
{
public:
	MasterData masterData;
	CPLEXMasterSolution masterSol;

	MasterProblem() : Problem("0") {}

	std::unique_ptr<Problem> clone() const override {
		return std::make_unique<MasterProblem>(*this);
	}

	void setMasterData(const MasterData& data) {
		masterData = data;
	}
	void setMasterSolution(const CPLEXMasterSolution& sol) {
		masterSol = sol;
	}
};

class Subproblem : public Problem {
public:
	SubproblemData subproblemData;
	CPLEXSubproblemSolution subproblemSol;

	Subproblem(std::string id, SubproblemData data)
		: Problem(std::move(id)), subproblemData(std::move(data)) {
	}

	std::unique_ptr<Problem> clone() const override {
		return std::make_unique<Subproblem>(*this);
	}

	void setSubproblemData(const SubproblemData& data) noexcept {
		subproblemData = data;
	}

	void setSubproblemSolution(const CPLEXSubproblemSolution& sol) {
		subproblemSol = sol;
	}
};

//class Subproblem : public Problem
//{
//public:
//	SubproblemData subproblemData;
//	CPLEXSubproblemSolution subproblemSol;
//
//	Subproblem(std::string id) : Problem(id) {}
//
//	std::unique_ptr<Problem> clone() const override {
//		return std::make_unique<Subproblem>(*this);
//	}
//
//	void setSubproblemData(const SubproblemData& data) {
//		subproblemData = data;
//	};
//	void setSubproblemSolution(const CPLEXSubproblemSolution& sol) {
//		subproblemSol = sol;
//	}
//};

class Logger {
public:
	Logger(const std::string& instanceName, bool enable = true)
		: enabled(enable) {
		if (enabled) {
			std::filesystem::path dir = std::filesystem::path("output") / instanceName;
			std::filesystem::create_directories(dir);
			std::filesystem::path filename = dir / ("log_" + instanceName + ".txt");
			logFile.open(filename, std::ios::out);
			logFile.rdbuf()->pubsetbuf(0, 0);  //  disable buffering entirely
		}
	}

	~Logger() {                                //  RAII: flush & close on destruction
		if (logFile.is_open()) {
			logFile.flush();
			logFile.close();
		}
	}

	void log(const std::string& message) {
		if (enabled && logFile.is_open()) {
			logFile << message;
			logFile.flush();                   //  flush after every write
		}
	}

private:
	bool enabled;
	std::ofstream logFile;
};


inline std::vector<std::unique_ptr<Problem>> deepCopy(
	const std::vector<std::unique_ptr<Problem>>& src
) {
	std::vector<std::unique_ptr<Problem>> dst;
	dst.reserve(src.size());
	for (const auto& problem : src) {
		dst.push_back(problem->clone());
	}
	return dst;
}

Problem* getProblemByID(const std::vector<std::unique_ptr<Problem>>& problems, const std::string& id);

void printProblems(const std::vector<std::unique_ptr<Problem>>& problems);