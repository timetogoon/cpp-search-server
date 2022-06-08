#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) { stop_words_.insert(word); }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }

    template <typename Predicate>
    vector<Document> FindTopDocuments(const string& raw_query, Predicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
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
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status, int rating) { return doc_status == status; });
    }

    int GetDocumentCount() const {
        return static_cast <int>(documents_.size());
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>
    vector<Document> FindAllDocuments(const Query& query, Predicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id,relevance,documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

template <typename T>
void AssertEqualImpl(const T& t, const string& t_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (!t) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << t_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertEqualImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertEqualImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename TestFunc>
void RunTestImpl(TestFunc t_func, const string& func) {
    cerr << func;
    t_func();
    cerr << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)  

// -------- Начало модульных тестов поисковой системы ----------
map<int, double> word_freqs_doc;

template <typename Documents, typename Term>
vector <double> ComputeTfIdfs(const Documents docs, const Term term) {
    vector <double> backwords;
    int id = 0;
    int num = 0;
    for (const auto& words : docs) {
        ++id;
        double freqTF = static_cast<double> (count(begin(words), end(words), term)) / (words.size());
        word_freqs_doc[id] += freqTF;
        if (freqTF != 0) {
            num++;
        }
    }
    for (int i = 1; i <= docs.size(); i++) {
        backwords.push_back(abs(word_freqs_doc.at(i) * log(num * 1.0 / word_freqs_doc.size())));
    }
    return backwords;
}


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь*/
void TestAddDocuments() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        ASSERT_EQUAL(server.GetDocumentCount(), 0u);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.GetDocumentCount(), 1u);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
}

void TestExludeDocumentsWithMinusWords()
{
    const int doc_id1 = 1;
    const string content1 = "a colorful parrot with green wings and red tail is lost"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc_id2 = 2;
    const string content2 = "a grey hound with black ears is found at the railway station"s;
    const vector<int> ratings2 = { 2, 3, 4 };
    const int doc_id3 = 3;
    const string content3 = "a white cat with long furry tail is found near the red square"s;
    const vector<int> ratings3 = { 3, 4, 5 };
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.SetStopWords(""s);
        const auto found_docs = server.FindTopDocuments("white green cat -long tail"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id1);
    }
}

void TestMatchDocument() {
    const int doc_id = 1;
    const string content = "a colorful parrot with green wings and red tail is lost"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.SetStopWords("is are was a an in the with near"s);
        const auto [words, status] = server.MatchDocument("white cat -long tail"s, doc_id);

        ASSERT_EQUAL(words.size(), 1u);
        ASSERT_EQUAL(words[0], "tail");
        ASSERT_EQUAL(static_cast<int>(status), static_cast<int>(DocumentStatus::ACTUAL));

        const auto [words1, status1] = server.MatchDocument("white cat -long -tail"s, doc_id);
        ASSERT_EQUAL(words1.size(), 0u);
    }
}

void TestSortRel() {
    const int doc_id1 = 1;
    const string content1 = "a colorful parrot with green wings and red tail is lost"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc_id2 = 2;
    const string content2 = "a grey hound with black ears is found at the railway station"s;
    const vector<int> ratings2 = { 2, 3, 4 };
    const int doc_id3 = 3;
    const string content3 = "a white cat with long furry tail is found near the red square"s;
    const vector<int> ratings3 = { 3, 4, 5 };
    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        server.SetStopWords("is are was a an in the with near"s);
        const auto found_docs = server.FindTopDocuments("white green cat long tail"s);
        ASSERT_EQUAL(found_docs.size(), 2u);
        const Document& doc0 = found_docs[0];
        const Document& doc1 = found_docs[1];
        ASSERT(doc0.relevance > doc1.relevance);
    }
}

void TestCalcRating() {
    const int doc_id = 3;
    const string content = "a white cat with long furry tail is found near the red square"s;
    const vector<int> ratings = { 3, 4, 5 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.SetStopWords("is are was a an in the with near"s);
        const auto found_docs = server.FindTopDocuments("white green cat long tail"s);
        const Document& doc0 = found_docs[0];
        auto rating_sum = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
        ASSERT_EQUAL(doc0.rating, rating_sum);
    }
}

void TestPredicate() {

    const int doc_id = 1;
    const string content = "a colorful parrot with green wings and red tail is lost"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 2;
    const string content2 = "a grey hound with black ears is found at the railway station tail"s;
    const vector<int> ratings2 = { 2, 3, 4 };

    const int doc_id3 = 3;
    const string content3 = "a white cat with long furry tail is found near the red square"s;
    const vector<int> ratings3 = { 3, 4, 5, 6 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings3);

        const auto found_docs = server.FindTopDocuments("white cat long tail"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 3 == 0; });
        ASSERT_EQUAL(found_docs[0].id, 3u);
        const auto found_docs1 = server.FindTopDocuments("white cat long tail"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::IRRELEVANT; });
        ASSERT_EQUAL(found_docs1[0].id, 2u);
        const auto found_docs2 = server.FindTopDocuments("white cat long tail"s, [](int document_id, DocumentStatus status, int rating) { return rating > 2; });
        ASSERT_EQUAL(found_docs2[0].id, 3u);
        ASSERT_EQUAL(found_docs2[1].id, 2u);
    }
}

void TestStatus() {

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "cat and dog in the home"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "cat and dog in the black box"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings);
        server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, 43u);
        const auto found_docs2 = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs2.size(), 1u);
        ASSERT_EQUAL(found_docs2[0].id, 44u);
        const auto found_docs3 = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
        ASSERT(found_docs3.empty());
    }
}

void TestCountingRelevansIsCorrect() {

    const int doc_id = 42;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
        const auto found_docs2 = server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
        auto wordsTest1 = SplitIntoWords(content);
        auto wordsTest2 = SplitIntoWords(content2);
        auto wordsTest3 = SplitIntoWords(content3);
        vector<vector<string>> testvec;
        testvec.push_back(wordsTest1);
        testvec.push_back(wordsTest2);
        testvec.push_back(wordsTest3);

        const auto& tf_idfs1 = ComputeTfIdfs(testvec, "кот"s);
        const auto& tf_idfs2 = ComputeTfIdfs(testvec, "пёс"s);

        ASSERT(abs(found_docs[0].relevance - tf_idfs1.at(1)) < EPSILON);
        ASSERT(abs(found_docs[1].relevance - tf_idfs1.at(0)) < EPSILON);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestExludeDocumentsWithMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestSortRel);
    RUN_TEST(TestCalcRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestCountingRelevansIsCorrect);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
}