#include "search_server.h"
#include <numeric>
#include <cmath>

using namespace std;

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
    return log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status,
    const vector<int>& ratings) {
    if (document_id <= -1) {
        throw invalid_argument("����� ��������� �������������"s);
    }
    else if (documents_.count(document_id) > 0) {
        throw invalid_argument("�������� � ����� ������� ��� ����������"s);
    }
    else if (!IsValidWord(document))
    {
        throw invalid_argument("� ������ ������� ���� �����-�� ����������"s);
    }
    else
    {
        const vector<string> words = SearchServer::SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
            document_words_freqs_[document_id][word] += static_cast<double>(count(words.begin(), words.end(), word)) / static_cast<int>(words.size());
        }

        documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
        documents_order_.insert(document_id);
    }
}

    vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const {
            return SearchServer::FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status; });       
    }

    vector<Document> SearchServer::FindTopDocuments(const string& raw_query) const {       
            return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);      
    }

int SearchServer::GetDocumentCount() const { 
        return static_cast<int>(documents_.size());    
}

set<int>::const_iterator SearchServer::begin() const {
    return documents_order_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return documents_order_.end();
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const {
    const Query query = SearchServer::ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsntRightWord(const string& text) const {
    if ((text[0] == '-' && text[1] == '-') || (text[0] == '-' && text[1] == 0)) {
        return false;
    }
    return true;
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

SearchServer::QueryWord SearchServer::ParseQueryWord(const string& text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    string word = text;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    return { word, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const string& text) const {
        SearchServer::Query query;
        if (!IsValidWord(text)) {
            throw invalid_argument("� ������ ������� ���� �����-�� ����������"s);
        }
        for (const string& word : SplitIntoWords(text)) {
            if (SearchServer::IsntRightWord(word)) {
                const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
                if (!query_word.is_stop) {
                    if (query_word.is_minus) {
                        query.minus_words.insert(query_word.data);
                    }
                    else {
                        query.plus_words.insert(query_word.data);
                    }
                }
            }
            else
            {
                query.minus_words.clear();
                query.plus_words.clear();
                throw invalid_argument("� ������ ������� ������: ���� ��� � ������ ������� ����� ������, ���� ����� ������ ��� ����"s);
            }
        }
        return query;
}

const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string, double> temp = {};
    if (documents_order_.count(document_id) == 0) {
        return temp;
    }
    else
    {
        return document_words_freqs_.at(document_id);
    }
}

void SearchServer::RemoveDocument(int document_id) {
    auto& docwords = SearchServer::GetWordFrequencies(document_id);
    for (auto& docword : docwords)
    {
        word_to_document_freqs_.at(docword.first).erase(document_id);        
    }
    document_words_freqs_.erase(document_id);
    documents_order_.erase(find(documents_order_.begin(), documents_order_.end(),document_id));
    documents_.erase(document_id);
}