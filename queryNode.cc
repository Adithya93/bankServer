#include "./queryNode.h"

queryNode::queryNode(char rel, std::string val, char attr) {
	this->rel = rel;
	this->attr = attr;
	//this->val = !isLogical() ? val : rel == 'a' ? " AND " : rel == 'o' ? " OR " : " NOT "; 
	children = new std::vector<queryNode*>();
	subTreeValid = true;
}

void queryNode::deleteNode() {
	for (std::vector<queryNode*>::iterator it = children->begin(); it < children->end(); it ++) {
		(*it)->deleteNode();
	}
	delete children;
	//delete this; // apparently considered bad practice, so call delete queryRequest from requestParser instead
}

bool queryNode::addChild(queryNode* child) { // only add if valid, else delete
	if (isValid() && child->isValid()) {
		this->children->push_back(child);
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
	return subTreeValid && ((logical && ((rel == 'n' && children->size() == 1) ||  ((rel == 'a' || rel == 'o') && children->size() != 1))) || (!logical && (rel == '>' || rel == '<' || rel == '=') && (attr == 'f' || attr == 't' || attr == 'a')));
}

std::string queryNode::getQueryString() { // in-order traversal; should only be called after children have been added
	if (!isValid()) {
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
	for (std::vector<queryNode*>::iterator it = children->begin(); it < children->end() - 1; it ++) {
		nodeQueryString += '(' + (*it)->getQueryString() + ')' + val; 
	}
	nodeQueryString += '(' + (*(children->end() - 1))->getQueryString() + ')';
	return nodeQueryString;
}

void queryNode::markInvalid() {
	subTreeValid = false;
}
