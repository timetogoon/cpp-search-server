#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    map<set<string_view>, int> word_to_document_freqs;   
    set<int> documents_to_delete;
    for (const int document_id : search_server) {    
        set<string_view> words;
        for (auto& wordsandfreqs : search_server.GetWordFrequencies(document_id)) 
        {
            words.insert(wordsandfreqs.first);
        }
        if (word_to_document_freqs.find(words) == word_to_document_freqs.end()) 
        {
            word_to_document_freqs[words] = document_id;
        }
        else if (document_id > word_to_document_freqs[words])
        {
            word_to_document_freqs[words] = document_id;
            documents_to_delete.insert(document_id);
        }
    }

    for (auto document : documents_to_delete)
    {
        std::cout << "Found duplicate document id " << document << std::endl;
        search_server.RemoveDocument(document);
    }
}
