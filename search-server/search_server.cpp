#include "search_server.h"
#include <numeric>
#include <cmath>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
    if (!IsValidWord(stop_words_text)) {
        throw invalid_argument("в словах запроса есть какие-то кракозябры"s);
    }
}

SearchServer::SearchServer(const string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
    if (!IsValidWord(stop_words_text)) {
        throw invalid_argument("в словах запроса есть какие-то кракозябры"s);
    }
}

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status,
    const vector<int>& ratings) {
    if (document_id <= -1) {
        throw invalid_argument("номер документа отрицательный"s);
    }
    else if (documents_.count(document_id) > 0) {
        throw invalid_argument("документ с таким номером уже существует"s);
    }
    else if (!IsValidWord(document))
    {
        throw invalid_argument("в словах запроса есть какие-то кракозябры"s);
    }
    else
    {
        const vector<string_view> words = SearchServer::SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string_view word : words) {
            word_to_document_freqs_[string(word)][document_id] += inv_word_count;
            document_words_freqs_[document_id][word] += static_cast<double>(count(words.begin(), words.end(), word)) / static_cast<int>(words.size());
        }

        documents_.emplace(document_id, DocumentData{ SearchServer::ComputeAverageRating(ratings), status });
        documents_order_.insert(document_id);
    }
}

vector<Document> SearchServer::FindTopDocuments(string_view query, DocumentStatus document_status) const {
    return FindTopDocuments(query, [document_status](int, DocumentStatus status, int) {
        return status == document_status;
        });
}

vector<Document> SearchServer::FindTopDocuments(string_view query) const {
    return FindTopDocuments(query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

set<int>::const_iterator SearchServer::begin() const {
    return documents_order_.cbegin();
}

set<int>::const_iterator SearchServer::end() const {
    return documents_order_.cend();
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
    std::execution::sequenced_policy policy, const string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    if (document_id < 0 || document_words_freqs_.count(document_id) == 0) {
        throw out_of_range("Документа с указанным id не существует");
    }
    Query query = SearchServer::ParseQuery(raw_query);

    vector<string_view> matched_words(query.plus_words.size());
    if (any_of(query.minus_words.begin(), query.minus_words.end(), [this, &document_id](const string_view minus_word) {
        return document_words_freqs_.at(document_id).count(minus_word); }))
    {
        return { {},documents_.at(document_id).status };
    }

        vector<string_view>::iterator end_copy = copy_if(query.plus_words.begin(), query.plus_words.end(),
            matched_words.begin(), [this, &document_id](const string_view plus_word) {
                return document_words_freqs_.at(document_id).count(plus_word);
            });

        matched_words.resize(distance(matched_words.begin(), end_copy));

        set<string_view> unique_words(matched_words.begin(), matched_words.end());

        return { vector<string_view> { unique_words.begin(), unique_words.end() }, documents_.at(document_id).status };        
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
    std::execution::parallel_policy policy, const string_view raw_query, int document_id) const {
    if (document_id < 0 || document_words_freqs_.count(document_id) == 0) {
        throw out_of_range("Документа с указанным id не существует");
    }
    const Query query = SearchServer::ParseQueryWithoutSort(raw_query);
    vector<string_view> matched_words(query.plus_words.size());

    if (any_of(policy, query.minus_words.begin(), query.minus_words.end(), [this, &document_id](const string_view minus_word) {
        return document_words_freqs_.at(document_id).count(minus_word); }))
    {
        return { {},documents_.at(document_id).status };
    }

    vector<string_view>::iterator end_copy = copy_if(policy, query.plus_words.begin(), query.plus_words.end(),
      matched_words.begin(), [this, &document_id](const string_view plus_word) {
        return document_words_freqs_.at(document_id).count(plus_word);
      });

    matched_words.resize(distance(matched_words.begin(), end_copy));
    set<string_view> unique_words(matched_words.begin(), matched_words.end());

    return { vector<string_view> { unique_words.begin(), unique_words.end() }, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsRightWord(const string& text) const {
    if ((text[0] == '-' && text[1] == '-') || (text[0] == '-' && text[1] == 0)) {
        return false;
    }
    return true;
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    string_view word = text;
    if (text.empty()) {
        throw invalid_argument("Текст пустой");
    }
    if (text[0] == '-') {
        if (text[1] == '-') {
            throw invalid_argument("Два минуса перед словом в запросе");
        }
        is_minus = true;
        word = word.substr(1);
    }
    return { word, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const string_view raw_query) const {
    SearchServer::Query query;
    if (!IsValidWord(raw_query)) {
        throw invalid_argument("в словах запроса есть какие-то кракозябры"s);
    }
    for (const string_view word : SplitIntoWords(raw_query)) {
        if (word == "-"s) {
            throw invalid_argument("После знака \" - \" отсутствует слово");
        }
        const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    sort(query.plus_words.begin(), query.plus_words.end());
    sort(query.minus_words.begin(), query.minus_words.end());
    auto last_minus = unique(query.minus_words.begin(), query.minus_words.end());
    auto last_plus = unique(query.plus_words.begin(), query.plus_words.end());

    // resize vectors by new size
    size_t newSize = last_minus - query.minus_words.begin();
    query.minus_words.resize(newSize);
    newSize = last_plus - query.plus_words.begin();
    query.plus_words.resize(newSize);
    return query;
}

SearchServer::Query SearchServer::ParseQueryWithoutSort(const string_view raw_query) const {
    SearchServer::Query query;
    if (!IsValidWord(raw_query)) {
        throw invalid_argument("в словах запроса есть какие-то кракозябры"s);
    }
    for (const string_view word : SplitIntoWords(raw_query)) {
        if (word == "-"s) {
            throw invalid_argument("После знака \" - \" отсутствует слово");
        }
        const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> temp = {};
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
        word_to_document_freqs_.at(string(docword.first)).erase(document_id);
    }
    document_words_freqs_.erase(document_id);
    documents_order_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id)
{
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id)
{
    auto& docwords = SearchServer::GetWordFrequencies(document_id);
    vector<const string_view*> temp(docwords.size());

    transform(policy, docwords.begin(), docwords.end(), temp.begin(), [&](const auto& pointer) {
        return (&pointer.first);
        });

    for_each(policy, temp.begin(), temp.end(), [&](const string_view* word) {
        word_to_document_freqs_[string(*word)].erase(document_id);
        });

    document_words_freqs_.erase(document_id);
    documents_order_.erase(document_id);
    documents_.erase(document_id);
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(SearchServer::GetDocumentCount() * 1.0 / word_to_document_freqs_.at(string(word)).size());
}