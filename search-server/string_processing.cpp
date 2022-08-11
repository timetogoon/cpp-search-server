#include "string_processing.h"

using namespace std;

/*vector<string> SplitIntoWords(const string& text) {
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
}*/

vector<string_view> SplitIntoWords(string_view str) {
    vector<string_view> result;
    str.remove_prefix(min(str.size(), str.find_first_not_of(" ")));
    if (str.empty()) {
        return result;
    }
    while (true) {
        size_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        if (space == str.npos) {
            break;
        }
        else {
            str.remove_prefix(space + 1);
            size_t not_space = str.find_first_not_of(' ');
            if (not_space != str.npos) {
                str.remove_prefix(not_space);
            }
            else {
                break;
            }
        }
    }
    return result;
}