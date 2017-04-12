all: bank-st bank-mt


bank-st: server-st.cc
	g++ -o bank-st server-st.cc bankRequestParser.cc bankBackend.cc bankDBHandler.cc bankResponseWriter.cc queryNode.cc -std=c++11 -lxerces-c -lpqxx -lpq

bank-mt: server.cc
	g++ -o bank-mt server.cc bankRequestParser.cc bankBackend.cc bankDBHandler.cc bankResponseWriter.cc queryNode.cc threadPool.cc -std=c++11 -lxerces-c -lpqxx -lpq

clean:
	rm -rf *.o* *~ bank-mt
