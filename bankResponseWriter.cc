#include "./bankResponseWriter.h"

	std::string parseErrorResponse() {


		return std::string(); // TEMP
	}

	std::string createSuccessResponse(std::string ref) {

		return std::string(); // TEMP
	}

	std::string createErrorResponse(std::string ref) {

		return std::string(); // TEMP
	}

	std::string balanceSuccessResponse(std::string ref, float balance) {
		
		return std::string(); // TEMP
	}

	std::string balanceErrorResponse(std::string ref) {

		return std::string(); // TEMP
	}

	std::string transferSuccessResponse(std::string ref) {

		return std::string(); // TEMP
	}

	std::string transferErrorResponseInsufficientFunds(std::string ref) {

		return std::string(); // TEMP
	}

	std::string transferErrorResponseInvalidAccount(std::string ref) {

		return std::string(); // TEMP
	}

	std::string queryResponse(std::string ref, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>* queryResults) {

		return std::string(); // TEMP
	}

	std::string constructResponse(std::vector<std::tuple<bool, std::string>>* createResults, std::vector<std::tuple<float, std::string>>* balanceResults, std::vector<std::tuple<bool, std::string>>* transferResults, std::vector<std::tuple<std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>>* queryResults) {

		return std::string(); // TEMP
	}
			 
