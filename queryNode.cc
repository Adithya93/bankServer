#include "./queryNode.h"

queryNode::queryNode(char rel, std::string val, char attr) {
	this->rel = rel;
	this->attr = attr;
	this->val = !isLogical() ? val : rel == 'a' ? " AND " : rel == 'o' ? " OR " : " NOT "; 
	children = new std::vector<queryNode*>();
	subTreeValid = true;
}

void queryNode::deleteNode() {
	for (std::vector<queryNode*>::iterator it = children->begin(); it < children->end(); it ++) {
		(*it)->deleteNode();
		delete *it;
	}
	delete children;
	//delete this; // apparently considered bad practice, so call delete queryRequest from requestParser instead
}

bool queryNode::addChild(queryNode* child) { // only add if valid, else delete
	//if (isValid() && child->isValid()) {
	if (child->isValid()) {
		this->children->push_back(child);
		//std::cout << "Added child " << child->getString() << " to parent " << getString() << "\n";
		return true;
	}
	else {
		//delete(child);
		std::cout << "Either the parent " + getString() + " is invalid or the child " + child->getString() + " is invalid; not adding to vector of children; setting query subtree to be invalid\n";
		subTreeValid = false;
		return false;
	}
}

char queryNode::getRel() {
	return rel;
}

char queryNode::getAttr() {
	return attr;
}

std::string queryNode::getVal() {
	return val;
}

std::vector<queryNode*>* queryNode::getChildren() {
	return children;
}

std::string queryNode::getString() { // should be called after children have been added
	return isLogical() ? val : std::string() + attr + rel + val; 
}

bool queryNode::isLogical() {
	return rel == 'a' | rel == 'o' |  rel == 'n';
}

bool queryNode::isValid() {
	bool logical = isLogical();
	/*
	if (!subTreeValid) {
		std::cout << "Subtree of " << getString() << " already invalid\n";
		return false;
	}
	if (logical) {
		//std::cout << "Verifying validity of logical query node " << rel << "\n";
		if (rel == 'n') {
			//std::cout << "Verifying that NOT operator only has 1 child\n";
			if (children->size() != 1) {
				std::cout << "NOT operator has " << children->size() << " children; invalid!\n";
				return false;
			}
			//std::cout << "Valid NOT operator\n";
			return true;
		}
		if (rel == 'a' || rel == 'o') {
			//std::cout << "Verifying that AND/OR operator does not have exactly 1 child\n";
			if (children->size() == 1) {
				std::cout << "AND/OR operator has 1 child, invalid!\n";
				return false;
			}
			return true;
		}
		std::cout << "Invalid logical operator " << rel << "; Invalid!\n";
		return false;
	}
	if (rel == '>' || rel == '<' || rel == '=') {
		//std::cout << "Valid relational operator " << rel << "\n";
		return true;
	}
	std::cout << "Unknown relational operator " << rel << "\n";
	return false;
	*/
	// Below One-Liner is equivalent to commented-out code above
	return subTreeValid && ((logical && (((rel == 'n') && (children->size() < 2)) ||  (((rel == 'a') || (rel == 'o')) && (children->size() != 1)))) || (!logical && ((rel == '>') || (rel == '<') || (rel == '=')) && ((attr == 'f') || (attr == 't') || (attr == 'a'))));
}

std::string queryNode::getQueryString() { // in-order traversal; should only be called after children have been added
	if (!isValid()) {
		//std::cout << "Query string of " << getString() << " is invalid, treating as empty string\n";
		subTreeValid = false;
		return "";
	}
	//std::string nodeQueryString;
	//if (!isLogical()) return nodeQueryString + attr + ' ' + rel + ' ' + val;
	if (!isLogical()) return getString(); // Base-case
	std::string nodeQueryString;
	if (children->size() == 0) { // special case
		if (rel == 'a') {
			std::cout << "Empty AND, treating as TRUE\n";
			return "TRUE";
		}
		if (rel == 'o') {
			std::cout << "Empty OR, treating as FALSE\n";
			return "FALSE";
		}
		else { // valid, logical, not a and not o, so must be n
			std::cout<< "Empty NOT, treating as FALSE\n";
			return "FALSE";
		}
	}
	if (rel == 'n') { // NOT query written before operand, unlike AND / OR which are written between operands
		return val + children->front()->getQueryString();
	}
	for (std::vector<queryNode*>::iterator it = children->begin(); it < children->end() - 1; it ++) {
		nodeQueryString += '(' + (*it)->getQueryString() + ')' + val; 
	}
	nodeQueryString += '(' + (*(children->end() - 1))->getQueryString() + ')';
	return nodeQueryString;
}

void queryNode::markInvalid() {
	subTreeValid = false;
}
