#pragma once

#include <vector>
#include <string>
#include <set>

//std::vector<std::string>SplitIntoWords(const std::string& text);

template <typename StringContainer>
std::set<std::string, std::less<>>MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::basic_string(str));
        }
    }
    return non_empty_strings;
}

std::vector<std::string_view>SplitIntoWords(std::string_view str);