#include <iostream>
#include <vector>

using namespace std;


class QueryOp {
	
	public:
		virtual vector<int> processOp(vector<int> inputSet) = 0; // Tried to do polymorphic version but virtual template not allowed :(
};

class LogOp: public QueryOp {
	public:
		int isOr;
		vector<QueryOp*>* children;

		vector<int> processOp(vector<int> inputSet) {
			printf("Size of input set: %lu\n", inputSet.size());
			if (!isOr) { // and
				printf("Processing AND operator with %lu children\n", children->size());
				for (vector<QueryOp*>::iterator it = children->begin(); it < children->end(); it ++) {
					inputSet = (*it)->processOp(inputSet);
					printf("Size of inputSet after processing: %lu\n", inputSet.size());
				}
				return inputSet;
			}
			else { // or
				printf("Processing OR operator with %lu children\n", children->size());
				vector<int> result;
				result.reserve(inputSet.size()); // max possible size after unions
				vector<int>::iterator resultIt;// = result.end();// = result.begin();

				if (children->empty()) {
					puts("OR Operator has no children, returning empty set");
					return result; // empty set
				}
				vector<int> oldResult;

				printf("Size of result after reserving: %lu\n", result.size());
				for (vector<QueryOp*>::iterator it = children->begin(); it < children->end(); it ++) {
					vector<int> childResult = (*it)->processOp(inputSet);
					printf("Size of childResult: %lu\n", childResult.size());
					puts("About to perform set union");
					sort(childResult.begin(), childResult.end());
					sort(oldResult.begin(), oldResult.end());
					resultIt = set_union(childResult.begin(), childResult.end(), oldResult.begin(), oldResult.end(), result.begin());//result.begin());
					printf("Difference in iterators: %lu\n", resultIt - result.begin());
					oldResult = vector<int>(result.begin(), resultIt); // Can avoid this copying? Otherwise result seems to be wrong :(
						// TO-DO : Refactor with std::swap of pointers instead?
				}
				return oldResult;
			}
			
		}

		LogOp(int isOr, vector<QueryOp*>* children) {
			this->isOr = isOr;
			this->children = children;
		}

		void addChild(QueryOp * q) {
			this->children -> push_back(q);
		}

	private:



};

class RelOp: public QueryOp {
	public:
		int queryFieldId; // 0 for from, 1 for to, 2 for amount
		int relation; // -1 for less, 0 for equals, 1 for greater
		int value;

		vector<int> processOp(vector<int> inputSet) {
			printf("Processing RelOp with queryFieldId %d, relation %d and value %d\n", queryFieldId, relation, value);
			vector<int> result = vector<int>();
			for (vector<int>::iterator it = inputSet.begin(); it < inputSet.end(); it ++) {
				if ((*it > value && relation == 1) || (*it < value && relation == -1)) {
					printf("Inserting %d\n", *it);
					result.insert(result.end(), *it);
					printf("No. of items CURRENTLY satisfying this RelOp: %lu\n", result.size());
				}		
				
			}
			printf("No. of items satisfying this RelOp: %lu\n", result.size());
			puts("RelOp returning vector:");
			for (vector<int>::iterator it = result.begin(); it < result.end(); it ++) {
				printf("%d, ", *it);
			}
			return result;
		}
		RelOp(int queryFieldId, int relation, int value) {
			this->queryFieldId = queryFieldId;
			this->relation = relation;
			this->value = value;
		}

};


int main() {
	RelOp r1 = RelOp(0, 1, 6);
	RelOp r2 = RelOp(0, -1, 10);
	vector<int> test = {2, 3, 6, 8, 10, 11};
	vector<QueryOp*> testVector = {&r1, &r2};
	LogOp l1 = LogOp(0, &testVector);
	LogOp l2 = LogOp(1, &testVector);
	vector <int> result1 = l1.processOp(test);
	printf("Result1 size: %lu\n", result1.size());
	for (vector<int>::iterator it = result1.begin(); it < result1.end(); it ++) {
		printf("%d; ", *it);
	}
	vector <int> result2 = l2.processOp(test);
	printf("Result2 size: %lu\n", result2.size());
	for (vector<int>::iterator it = result2.begin(); it < result2.end(); it ++) {
		printf("%d; ", *it);
	}
	printf("\n");
	vector <QueryOp*> testVector2 = {&l2, &r2};
	LogOp l3 = LogOp(0, &testVector2);
	vector<int> result3 = l3.processOp(test);
	printf("Result3 size: %lu\n", result3.size());
	for (vector<int>::iterator it = result3.begin(); it < result3.end(); it ++) {
		printf("%d; ", *it);
	}
	printf("\n");
	vector<QueryOp*> testVector3 = {&l1, &l3};
	LogOp l4 = LogOp(1, &testVector3);
	vector<int> result4 = l4.processOp(test);
	printf("Result4 size: %lu\n", result4.size());
	for (vector<int>::iterator it = result4.begin(); it < result4.end(); it ++) {
		printf("%d; ", *it);
	}
	printf("\n");
	return 0;
}