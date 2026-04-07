# Allocation and Reallocation a Freight Overbooking Problem - a Logic Based Benders Decomposition Approach

This project aims to solve an Allocation and Reallocation Problem, given an initial allocation recourse decisions can be made based on the revealed future. This project applies to a particular version of the Allocation and Reallocation Problem, where the uncertainty depends directly on the intial decision.

## Algorithm

Step 1. Solve Master Problem;  
Step 2. Calculate Scenarios;  
Step 3. Solve Subproblem Tree;
Step 4. If not optimal, go to Step 1;

## Project Structure

/project-root  
│── src/  
│ ├── main.cpp  
│ ├── SolveProblems.cpp  
│ ├── GPBD.cpp  
│ ├── ReadParameters.cpp  
│ └── debugSolveSubproblem.cpp  
|  
│ │── include/  
│ ├── ReadParameters.h  
│ ├── SolveProblems.h  
│ └── GPBD.h  
|  
│ │── instances/  
│ ├── problemdata1.dat  
│ └── problemdata2.dat  
|  
│ │── solutions/  
│ └── solution.txt  
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

nN - Number of high-capcity transport services;  
nR - Number of requests;  
a - revenue per booked container;  
b - cost overbooking per container;  
c - cost of re-allocation per container;  
d - cost of using truck;  
e - cost of underbooking per container;  
Q - vector of size nN, capacity of each transport service;  
q - vector of size nR, number of containers per request;  
p - vector of size nR, probability of request arrive on time;  
t - vector of size nR, to which high-capcity transport service each request was allocated to. If 0, not allocated. If 1 < = t_r <= nN, request allcoated to corresponding high-capcity transport service;  
theta - matrix of size nN x nR, if request can be allocated or re-allocated to high-cacity transport service.

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
