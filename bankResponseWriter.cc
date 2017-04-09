#include "./bankResponseWriter.h"


	bankResponseWriter::bankResponseWriter() {
		unparseableError = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<error>Unparseable document. Please ensure proper formatting and correct length specification in header.</error>\n");
	}

	std::string bankResponseWriter::getParseErrorResponse() {
    	return addHeaderString(unparseableError);
	}

	std::string bankResponseWriter::createSuccessResponse(std::string ref) {
		return "<success ref=\"" + ref + "\">created</success>\n";
	}

	std::string bankResponseWriter::createErrorResponse(std::string ref) {
		return "<error ref=\"" + ref + "\"> Already exists </error>"; // TEMP
	}

	std::string bankResponseWriter::balanceSuccessResponse(std::string ref, float balance) {
		return "<success ref=\"" + ref + "\">" + std::to_string(balance) + "</success>\n";
	}

	std::string bankResponseWriter::balanceErrorResponse(std::string ref) {
		return "<error ref=\"" + ref + "\"> Balance does not exist </error>\n";
	}

	std::string bankResponseWriter::transferSuccessResponse(std::string ref) {
		return "<success ref=\"" + ref + "\">transferred</success>\n";
	}

	std::string bankResponseWriter::transferErrorResponseInsufficientFunds(std::string ref) {
		return "<error ref=\"" + ref + "\"> Insufficient Funds </error>\n";
	}

	std::string bankResponseWriter::transferErrorResponseInvalidAccount(std::string ref) {
		return "<error ref=\"" + ref + "\"> Invalid Account </error>\n";
	}

	std::string bankResponseWriter::querySuccessResponse(std::string ref, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>* queryResults) {
		//std::cout << "Appending successful query result\n";
		std::string header("<results ref=" + ref + "\">\n");
		std::string footer("</results>\n");
		std::string body;
		for (std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>::iterator it = queryResults->begin(); it < queryResults->end(); it ++) {
			std::tuple<unsigned long, unsigned long, float, std::vector<std::string>> transferTup = *it;
			body += "<transfer>\n";
			body += "<from>" + std::to_string(std::get<0>(transferTup)) + "</from>\n";
			body += "<to>" + std::to_string(std::get<1>(transferTup)) + "</to>\n";
			body += "<amount>" + std::to_string(std::get<2>(transferTup)) + "</amount>\n";
			body += "<tags>\n"; // include <tags></tags> even when no tags?
			std::vector<std::string> tags = std::get<3>(transferTup);
			for (std::vector<std::string>::iterator it2 = tags.begin(); it2 < tags.end(); it2 ++) {
				body += "<tag>" + *it2 + "</tag>\n";
			}
			body += "</tags>\n";
			body += "</transfer>\n";
		}
		//std::string result(header + body + footer);
		//std::cout << "querySuccessResponse string: " << result << "\n";
		//return result;
		return header + body + footer;
	}

	std::string bankResponseWriter::queryErrorResponse(std::string ref) {
		return "<error ref=\"" + ref + "\"> Invalid Query </error>";
	}

	// Only for successfully parsed requests
	//std::string bankResponseWriter::constructResponse(std::vector<std::tuple<bool, std::string>>* createResults, std::vector<std::tuple<float, std::string>>* balanceResults, std::vector<std::tuple<bool, std::string>>* transferResults, std::vector<std::tuple<std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>>* queryResults) {
	std::string bankResponseWriter::constructResponse(std::vector<std::tuple<bool, std::string>>* createResults, std::vector<std::tuple<float, std::string>>* balanceResults, std::vector<std::tuple<int, std::string>>* transferResults, std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>>* queryResults) {
		std::string opening("<?xml version=\"1.0\" encoding=\"UTF-8\" ?><results>");
		std::string body;

		for (std::vector<std::tuple<bool, std::string>>::iterator it = createResults->begin(); it < createResults->end(); it ++) {
			std::tuple<bool, std::string> nextCreation = *it;
			if (std::get<0>(nextCreation)) {
				body.append(createSuccessResponse(std::get<1>(nextCreation)));				
			}
			else {
				body.append(createErrorResponse(std::get<1>(nextCreation)));
			}
		}
		for (std::vector<std::tuple<float, std::string>>::iterator it = balanceResults->begin(); it < balanceResults->end(); it ++) {
			std::tuple<float, std::string> nextBalance = *it;
			float balance = std::get<0>(nextBalance);
			if (balance >= MIN_BALANCE) { 
				body.append(balanceSuccessResponse(std::get<1>(nextBalance), balance));
			}
			else { // flags error
				body.append(balanceErrorResponse(std::get<1>(nextBalance)));
			}
		}
		//for (std::vector<std::tuple<bool, std::string>>::iterator it = transferResults->begin(); it < transferResults->end(); it ++) {
		for (std::vector<std::tuple<int, std::string>>::iterator it = transferResults->begin(); it < transferResults->end(); it ++) {		
			std::tuple<int, std::string> nextTransfer = *it;
			int transferResult = std::get<0>(nextTransfer);
			if (transferResult == TRANSFER_SUCCESS) {
				body.append(transferSuccessResponse(std::get<1>(nextTransfer)));
			}
			else if (transferResult == INVALID_ACCOUNT) {
				body.append(transferErrorResponseInvalidAccount(std::get<1>(nextTransfer)));
			}
			else if (transferResult == INSUFFICIENT_FUNDS) {
				body.append(transferErrorResponseInsufficientFunds(std::get<1>(nextTransfer)));
			}
		}

		for (std::vector<std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>>::iterator it = queryResults->begin(); it < queryResults->end(); it ++) {
			std::tuple<bool, std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>> queryResult = *it;
			if (!std::get<0>(queryResult)) { // error
				body.append(queryErrorResponse(std::get<1>(queryResult)));
			}
			else {
				//std::cout << "Appending query success response\n";
				body.append(querySuccessResponse(std::get<1>(queryResult), &std::get<2>(queryResult)));
			}
		}
		std::string closing("</results>");
		opening.append(body);
		opening.append(closing);
		return addHeaderString(opening);
	}

	std::string bankResponseWriter::hostToNetLong64(unsigned long hostLong) {
		unsigned int most4 = htonl((uint32_t)hostLong);
		unsigned int least4 = htonl((uint32_t)*(((int*)&hostLong) + 1));
		unsigned long netLongMost4 = ((unsigned long)most4) << (8 * (int)(sizeof(int)));
		unsigned long netLongLeast4 = ((unsigned long)least4);
		unsigned long netLong = netLongMost4 | netLongLeast4;
		std::cout << "Net long: " << netLong << "\n";
		char* byteChars = (char*)malloc(sizeof(unsigned long) + 1);
		memset(byteChars, 0, sizeof(unsigned long) + 1);
		strcpy(byteChars, (char*)&netLong);
		std::string byteString(byteChars);
		free(byteChars);
		return byteString;
	}


	std::string bankResponseWriter::addHeaderString(std::string responseBody) {
		unsigned long responseLen = (unsigned long)responseBody.size();
		std::cout << "Length of response body: " << responseLen << "\n";
		std::string responseLenStr = hostToNetLong64(responseLen);
    	std::cout<< "Response length string: " << responseLenStr << "\n";
    	responseLenStr.append(responseBody);
    	return responseLenStr;
	}
			 
