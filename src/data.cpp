#include "../include/data.h"

void Problem::setParentProblem(Problem*& parent) {
    parentProblem = parent;
}

void Problem::addChildProblem(Problem*& child) {
    childrenProblems.push_back(child);
}

void Problem::setLevel(int l) {
    level = l; 
}

void Problem::setScenario(int s) { 
    scenario = s; 
}

std::string Problem::getProblemID() const {
    return problemID;  // or `id` if that’s the member name
}

Problem* getProblemByID(const std::vector<std::unique_ptr<Problem>>& problems, const std::string& id)
{
    for (const auto& problemPtr : problems) {
        if (problemPtr->problemID == id) {
            return problemPtr.get();
        }
    }
	return nullptr;
}

Problem* Problem::getParentProblem() {
	return parentProblem;
}
std::vector<Problem*> Problem::getChildrenProblems() {
	return childrenProblems;
}

void printProblems(const std::vector<std::unique_ptr<Problem>>& problems) {
    for (const auto& p : problems) {
        std::cout << "problemID: " << p->problemID;

        // Parent
        if (p->parentProblem) {
            std::cout << ", parentProblemID: " << p->parentProblem->problemID;
        }
        else {
            std::cout << ", parentProblemID: none";
        }

        // Children
        std::cout << ", childProblemIDs: [";
        for (size_t i = 0; i < p->childrenProblems.size(); ++i) {
            std::cout << p->childrenProblems[i]->problemID;
            if (i + 1 < p->childrenProblems.size()) {
                std::cout << ", ";
            }
        }
        std::cout << "]";
        std::cout << ", level: " << p->level
            << ", scenario: " << p->scenario
			<< std::endl;
    }
}