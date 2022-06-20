#pragma once

#include "search_server.h"
#include <vector>
#include <string>
#include <deque>

class RequestQueue {
    const SearchServer& member;
public:
    RequestQueue(const SearchServer& search_server)
        : member(search_server)
    {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);    

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {   
        int request_time = 0;
        int requests = 0;
    };
    std::deque<QueryResult> requests_;

    int empty_request;

    const static uint64_t min_in_day_ = 1440;

    uint64_t counter_min_ = 0;

    void AddRequest(int request_size);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto request = member.FindTopDocuments(raw_query, document_predicate);
    RequestQueue::AddRequest(request.size());
    return request;
}
