
#include "test_example_functions.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include <set>
#include <numeric>

using namespace std;
using namespace std::string_literals;

// -------- Начало модульных тестов поисковой системы ----------
// 
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(" "s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);       
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
        SearchServer server(" "s);
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
        SearchServer server(" "s);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);        
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
        SearchServer server("is are was a an in the with near"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);        
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
        SearchServer server("is are was a an in the with near"s);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);        
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
        SearchServer server("is are was a an in the with near"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
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
        SearchServer server(" "s);
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
        SearchServer server(" "s);
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
        SearchServer server(" "s);
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

void TestRequests() {
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << endl;
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
}

void TestBeginEndSearchServer() {
    SearchServer server(" "s);
    
    vector<int> doc = {};

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "пушистый кот пушистый хвост"s;
    const vector<int> ratings2 = { 3, 3, 3 };

    const int doc_id3 = 44;
    const string content3 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings3 = { 4, 4, 4 };

    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings);

    for (const int document_id : server) {
        doc.push_back(document_id);
    }

    ASSERT_EQUAL(doc[0], 42u);
    ASSERT_EQUAL(doc[1], 43u);
    ASSERT_EQUAL(doc[2], 44u);
}

void TestGetWordFrequencies() {    
    SearchServer server("in the"s);

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    ASSERT(server.GetWordFrequencies(0).empty());

    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

    auto& test1 = server.GetWordFrequencies(42);
    ASSERT_EQUAL(test1.at("cat"s), static_cast<double>(0.5));
    ASSERT_EQUAL(test1.at("city"s), static_cast<double>(0.5));
}

void TestRemoveDocument() {
    SearchServer server("in the"s);

    server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    auto test = server.GetDocumentCount();
    ASSERT_EQUAL(test,2u);

    server.RemoveDocument(2);
    auto test1 = server.GetDocumentCount();
    ASSERT_EQUAL(test1, 1u);
}

void TestRemoveDuplicates() {    

    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // дубликат документа 2, будет удалён
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // отличие только в стоп-словах, считаем дубликатом
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, считаем дубликатом документа 1
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // добавились новые слова, дубликатом не является
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, { 1, 2 });

    // есть не все слова, не является дубликатом
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, { 1, 2 });

    // слова из разных документов, не является дубликатом
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, { 1, 2 });

    cout << endl;
    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
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
    RUN_TEST(TestRequests);
    RUN_TEST(TestBeginEndSearchServer);
    RUN_TEST(TestGetWordFrequencies);
    RUN_TEST(TestRemoveDocument);
    RUN_TEST(TestRemoveDuplicates);
}

// --------- Окончание модульных тестов поисковой системы -----------