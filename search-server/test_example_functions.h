#pragma once

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include "search_server.h"

#define ASSERT(expr) AssertEqualImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertEqualImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

#define RUN_TEST(func) RunTestImpl((func), #func) 

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    using namespace std;
    using namespace std::string_literals;
    if (t != u) {
        cout << std::boolalpha;
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

template <typename T>
void AssertEqualImpl(const T& t, const std::string& t_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint) {
    using namespace std;
    using namespace std::string_literals;
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

template <typename TestFunc>
void RunTestImpl(TestFunc t_func, const std::string& func) {
    using namespace std;
    using namespace std::string_literals;
    cerr << func;
    t_func();
    cerr << " OK"s << endl;
} 

template <typename Documents, typename Term>
std::vector <double> ComputeTfIdfs(const Documents docs, const Term term) {
    std::map<int, double> word_freqs_doc;
    std::vector <double> backwords;
    int id = 0;
    int num = 0;
    for (const auto& words : docs) {
        ++id;
        double freqTF = static_cast<double> (std::count(std::begin(words), std::end(words), term)) / (words.size());
        word_freqs_doc[id] += freqTF;
        if (freqTF != 0) {
            num++;
        }
    }
    for (int i = 1; i <= docs.size(); i++) {
        backwords.push_back(std::abs(word_freqs_doc.at(i) * std::log(num * 1.0 / word_freqs_doc.size())));
    }
    return backwords;
}

void TestSearchServer();
void TestExcludeStopWordsFromAddedDocumentContent();
void TestAddDocuments();
void TestExludeDocumentsWithMinusWords();
void TestMatchDocument();
void TestSortRel();
void TestCalcRating();
void TestPredicate();
void TestStatus();
void TestCountingRelevansIsCorrect();
void TestRequests();
void TestBeginEndSearchServer();
void TestGetWordFrequencies();
void TestRemoveDocument();