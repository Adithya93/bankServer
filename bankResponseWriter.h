#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <iostream>
#include <vector>

class bankResponseWriter {

	public:
		std::string parseErrorResponse();

		std::string createSuccessResponse(std::string ref);

		std::string createErrorResponse(std::string ref);

		std::string balanceSuccessResponse(std::string ref, float balance);

		std::string balanceErrorResponse(std::string ref);

		std::string transferSuccessResponse(std::string ref);

		std::string transferErrorResponseInsufficientFunds(std::string ref);

		std::string transferErrorResponseInvalidAccount(std::string ref);

		std::string queryResponse(std::string ref, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>* queryResults);

		std::string constructResponse(std::vector<std::tuple<bool, std::string>>* createResults, std::vector<std::tuple<float, std::string>>* balanceResults, std::vector<std::tuple<bool, std::string>>* transferResults, std::vector<std::tuple<std::string, std::vector<std::tuple<unsigned long, unsigned long, float, std::vector<std::string>>>>>* queryResults);
				 
};