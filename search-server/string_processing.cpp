#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string_view text) {
    std::vector<std::string> words;
    words.reserve(500); //не более 500 слов в поисковом запросе, включая минус-слова;
    std::string word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);

    }
    words.shrink_to_fit();
    words.push_back("hello");
    return words;
}
