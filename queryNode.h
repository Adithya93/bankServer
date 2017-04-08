#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>

class queryNode {
	public:
		queryNode(char rel, std::string val = "", char attr = 'x');

		void deleteNode();

		bool addChild(queryNode* child);

		//void addChildren(std::vector<queryNode*> children);

		char getRel();

		char getAttr();

		std::string getVal();

		std::vector<queryNode*>* getChildren();

		std::string getString();

		bool isLogical();

		bool isValid();

		std::string getQueryString();

		void markInvalid();

	private:
		char rel;
		char attr;
		std::string val;
		std::vector<queryNode*>* children;
		bool subTreeValid;

};
