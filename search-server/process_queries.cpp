#include "process_queries.h"
#include <execution>

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
	vector<vector<Document>> qresult(queries.size());
		transform(execution::par, queries.begin(), queries.end(), qresult.begin(), [&search_server](const string& query) {
			return search_server.FindTopDocuments(query);
		});
		return qresult;
}
vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
	vector<Document> qresult;
	for (const auto& documents : ProcessQueries(search_server, queries)) {
		qresult.insert(qresult.end(), documents.begin(), documents.end());
	}
	return qresult;
}