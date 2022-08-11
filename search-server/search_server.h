#pragma once

#include <algorithm>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <stdexcept>
#include <execution>
#include <thread>
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

using namespace std::string_literals;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename Policy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;

    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentStatus status) const;

    template <typename Policy>
    std::vector<Document> FindTopDocuments(Policy& policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    int ComputeAverageRating(const std::vector<int>& ratings);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(
        std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);

    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    const std::set<std::string, std::less<>> stop_words_;

    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    std::map<int, std::map<std::string_view, double>> document_words_freqs_;

    std::map<int, DocumentData> documents_;

    std::set<int> documents_order_;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    Query ParseQuery(const std::string_view text) const;

    Query ParseQueryWithoutSort(const std::string_view raw_query) const;

    QueryWord ParseQueryWord(const std::string_view text) const;

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    bool IsStopWord(const std::string_view word) const;

    bool IsRightWord(const std::string& text) const;

    static bool IsValidWord(const std::string_view word);

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename Policy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(Policy& policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    for (const auto& wordFromStops : stop_words_) {
        if (!IsValidWord(wordFromStops)) {
            throw std::invalid_argument("в словах запроса есть какие-то кракозябры"s);
        }
    }
}

template <typename Policy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(Policy& policy, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(std::thread::hardware_concurrency());
    
    //for plus words
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
        [&document_predicate, &policy, &document_to_relevance, this](const auto& word_view) {
            if (word_to_document_freqs_.count(std::string(word_view)) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word_view);
                std::for_each(policy, word_to_document_freqs_.at(std::string(word_view)).begin(),
                    word_to_document_freqs_.at(std::string(word_view)).end(),
                    [&document_to_relevance, &document_predicate, &inverse_document_freq, this]
                (const auto& pair)
                    {
                        const auto& document_data = documents_.at(pair.first);
                        if (document_predicate(pair.first, document_data.status, document_data.rating)) {
                            document_to_relevance[pair.first].ref_to_value += pair.second * inverse_document_freq;
                        }
                    });
            }
        });

    //for minus words
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(),
        [&document_predicate, &policy, &document_to_relevance, this](const auto& word_view) {
            if (word_to_document_freqs_.count(std::string(word_view)) != 0) {
                std::for_each(policy, word_to_document_freqs_.at(std::string(word_view)).begin(),
                    word_to_document_freqs_.at(std::string(word_view)).end(),
                    [&document_to_relevance]
                (const auto& pair)
                    {                        
                        document_to_relevance.Erase(pair.first);
                    });
            }
        });
    
    auto result = document_to_relevance.BuildOrdinaryMap();

    std::vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : result) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;    
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments( policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status; });
}

template <typename Policy>
std::vector<Document> SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename Policy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
}
