# Multi-Period Multi-Resource Stochastic Generalized Assignment Problem - Exact Approach

This project aims to solve the Multi-Period Multi-Resource Stochastic Generalized Assignment Problem (MP-MRSGAP) by using a Nested Logic-Based Benders Decomposition (NLBBD). The problem consists formulates an assignment in a multi-period setting and in two stages – a planning stage where all agents are included and a recourse stage for each period. During the planning stage, jobs are assigned to agents, and the model allows resource assignments to exceed the resources available to each agent. Depending on the realization of the uncertainty, in the second stage, a recourse decision can be made to re-assign jobs to other agents

## Algorithm

Step 1. Solve Master Problem;  
Step 2. Calculate Scenarios;  
Step 3. Solve Subproblem Tree;
Step 4. If not optimal, go to Step 1;

## Project Structure

/project-root  
│── src/  
│ ├── data.cpp  
│ ├── main.cpp  
│ ├── mainAlgorithm.cpp  
│ ├── preprocessing.cpp  
│ └── solveProblems.cpp  
|  
│ │── include/  
│ ├── data.h  
│ ├── mainAlgorithm.h  
│ ├── preprocessing.h  
│ └── solveProblems.h  
|  
│ │── instances/  
│ └── benchmark_3_10_1-2.dat  
|  
│ │── output/  
│ │── benchmark_3_10_1-2/  
│ ├── Bounds_benchmark_3_10_1-2.txt
│ ├── FinalSolution_benchmark_3_10_1-2.txt
│ ├── SolutionHardCapacity_benchmark_3_10_1-2.txt
│ └── SolutionSummary_benchmark_3_10_1-2.txt
|  
| │── README.md

## Requirements

- **IBM ILOG CPLEX Optimization Studio** (22.1.1 or later recommended)
- A C++17 capable compiler:
  - MSVC (Visual Studio 2019/2022)
  - g++ 7.3+ or clang++ 6.0+

## Build Instructions

### Windows (Visual Studio)

1. Install **CPLEX Optimization Studio**.  
   Example path: C:\IBM\ILOG\CPLEX_Studio2211\

2. In Visual Studio:

- Open project properties → **Configuration Properties → C/C++ → General → Additional Include Directories**  
  Add:

  ```
  C:\IBM\ILOG\CPLEX_Studio2211\cplex\include
  C:\IBM\ILOG\CPLEX_Studio2211\concert\include
  ```

- Go to **Linker → General → Additional Library Directories**  
  Add:

  ```
  C:\IBM\ILOG\CPLEX_Studio2211\cplex\lib\x64_windows_vs2017\stat_mda
  C:\IBM\ILOG\CPLEX_Studio2211\concert\lib\x64_windows_vs2017\stat_mda
  ```

- Go to **Linker → Input → Additional Dependencies**  
  Add:

  ```
  cplex2211.lib
  ilocplex.lib
  concert.lib
  ```

- Under **C/C++ → Language → C++ Language Standard**, set:
  ```
  ISO C++17 Standard (/std:c++17)
  ```

3. Build and run the project.

## Instances Parameters

nN - Number of agents;  
nR - Number of requests;
nR - Number of resoruces;  
a - revenue assigning request r to agent i;  
c - cost of reassigned of request r from agent i to agent j;  
Q - total resources available of type h in agent i;  
q - resources of type h of request r;  
p - probability of request r materializing;  
t - initial assingment solution;

## Navigation the Tree (Fast-Forward Heuristic)

### Problems Structure

- **Problem** (base class)
  - **MasterProblem** (subclass of `Problem`)
  - **Subproblem** (subclass of `Problem`)

### Going Up the Tree

- **Leaf child problems:**  
  Move to the previous subproblems.

- **All child solutions unchanged:**
  - If child subproblems are at the first level, return to the **MasterProblem**.
  - Otherwise, return to the previous subproblems.

- **Infeasible child subproblem in a branch:**
  - Skip the remaining subproblems in that branch.
  - Solve the remaining child problems in other branches.
  - Then move to the previous subproblems.

### Going Down the Tree

- Proceed downwards when at least one of the child subproblems optimal solutions does not stay the same.

## Authors

- [@CambotasDeMedeiros](https://github.com/CambotasDeMedeiros)
