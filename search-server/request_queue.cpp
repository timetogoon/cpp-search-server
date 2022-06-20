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
    //return static_cast<int>(std::count_if(requests_.begin(), requests_.end(), [](QueryResult i) { return i.requests == 0; }));
    return empty_request;
}

void RequestQueue::AddRequest(int request_size) {
    ++counter_min_;
    if (counter_min_ > min_in_day_)
    {
        auto& request = requests_.front();
        if (request.requests == 0) {
            --empty_request;
        }
        requests_.pop_front();
    }    
    if (request_size == 0) {
        ++empty_request;
        RequestQueue::requests_.push_back({ {static_cast<int>(counter_min_)}, { 0 } });
    }
    else
    {
        RequestQueue::requests_.push_back({ {static_cast<int>(counter_min_)},{ 1 } });
    }
}