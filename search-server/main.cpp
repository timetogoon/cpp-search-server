#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include <iostream>

using namespace std;
using namespace std::string_literals;



int main() {
    setlocale(LC_ALL, "Russian");
    SearchServer search_server("и в на"s);
    try {
        search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
        search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
        search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

        cout << "ACTUAL by default:"s << endl;
        for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
            cout << document;
        }

        cout << "BANNED:"s << endl;
        for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
            cout << document;
        }

        cout << "Even ids:"s << endl;
        for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
            cout << document;
        }
        search_server.FindTopDocuments("пушистый --пес");
        return 0;
    }    
    catch (const std::exception& e) {
        std::cout << "Ошибка: "s << e.what() << std::endl;
    }
}

