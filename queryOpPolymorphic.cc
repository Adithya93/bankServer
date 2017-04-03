#include <iostream>
#include <map>
#include <unordered_set>
#include <vector>
#include <queue>

using namespace std;


class QueryOp {
	
	public:
		template <class T>
		vector<T> processOp(vector<T> inputSet) {
		
			//const = 0; // pure virtual function
			return vector<T>();
		} 
};

class LogOp: public QueryOp {
	public:
		int isOr;
		vector<QueryOp*>* children;
		template <class T>
		vector<T> processOp(vector<T> inputSet) {
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
				vector<T> result = vector<T>();
				//unordered_set<T>::iterator resultIt = result.begin();
				typename vector<T>::iterator resultIt = result.begin();
				if (children->empty()) {
					puts("OR Operator has no children, returning empty set");
					return result; // empty set
				}
				//for (vector<QueryOp*>::iterator it = children->begin(); it < children->end(); it ++) {
				//for (const auto& elem : result) {
				for (vector<QueryOp*>::iterator it = children->begin(); it < children->end(); it ++) {
					//unordered_set<T> childResult = (*it)->processOp(inputSet);
					vector<T> childResult = (*it)->processOp(inputSet);
					//vector<T> childResult = elem;//(*it)->processOp(inputSet);
					resultIt = set_union(childResult.begin(), childResult.end(), result.begin(), result.end(), resultIt);
					printf("Iterator address after processing: %p\n", resultIt);
					printf("Item at iterator after processing: %d\n", (int)*resultIt);
					printf("Size of result after processing: %lu\n", result.size());
				}
				return result;
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
		template <class T>
		vector<T> processOp(vector<T> inputSet) {
			printf("Processing RelOp with queryFieldId %d, relation %d and value %d\n", queryFieldId, relation, value);
			vector<T> result = vector<T>();
			//for (unsigned i = 0; i < inputSet.bucket_count(); i ++) {
			//	for (auto it = inputSet.begin(i); it < inputSet.end(i); it ++) {				
			for (typename vector<T>::iterator it = inputSet.begin(); it < inputSet.end(); it ++) {
				if ((*it > value && relation == 1) || (*it < value && relation == -1)) {
					printf("Inserting %d\n", *it);
					result.insert(*it);
				}		
				
			}
			printf("No. of items satisfying this RelOp: %lu\n", result.size());
			return result;
			//return NULL;
		}
		RelOp(int queryFieldId, int relation, int value) {
			this->queryFieldId = queryFieldId;
			this->relation = relation;
			this->value = value;
		}

};


int main() {
	RelOp r1 = RelOp(0, 1, 5);
	RelOp r2 = RelOp(0, -1, 9);
	//unordered_set<int> test = {2, 3, 6, 8, 10};
	vector<int> test = {2, 3, 6, 8, 10};
	vector<QueryOp*> testVector = {&r1, &r2};//vector<QueryOp*>(&r1, &r2);
	LogOp l1 = LogOp(0, &testVector);
	LogOp l2 = LogOp(1, &testVector);
	//unordered_set<int> result1 = l1.processOp(test);
	vector <int> result1 = l1.processOp(test);
	printf("Result1 size: %lu\n", result1.size());
	//unordered_set<int> result2 = l2.processOp(test);
	vector <int> result2 = l2.processOp(test);
	printf("Result2 size: %lu\n", result2.size());
	return 0;
}