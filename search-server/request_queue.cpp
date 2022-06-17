#include "request_queue.h"

using namespace std;

vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    auto request = member.FindTopDocuments(raw_query, status);
    AddRequest(static_cast<int>(request.size()));
    return request;
}

vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    auto request = member.FindTopDocuments(raw_query);
    AddRequest(static_cast<int>(request.size()));
    return request;
}

int RequestQueue::GetNoResultRequests() const {
    return static_cast<int>(std::count_if(requests_.begin(), requests_.end(), [](QueryResult i) { return i.requests == 0; }));
}

void RequestQueue::AddRequest(int request_size) {
    ++counter_min_;
    if (counter_min_ > min_in_day_)
    {
        requests_.pop_front();
    }
    if (request_size == 0) {

        requests_.push_back(0);
    }
    else
    {
        requests_.push_back(1);
    }
}