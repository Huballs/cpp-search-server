#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server){
    std::set<std::vector<std::string>> words_to_doc;
    std::vector<int> ids_to_remove;

    for(const auto id : search_server){
            const auto word_freq = search_server.GetWordFrequencies(id);
            std::vector<std::string> words;
            for(const auto& [word, _] : word_freq){
                words.push_back(word);
            }
            const auto [_, is_inserted] = words_to_doc.emplace(words);
            if(!is_inserted) ids_to_remove.push_back(id);
    }

    for(const auto id : ids_to_remove){
        search_server.RemoveDocument(std::execution::par, id);
        std::cout << "Found duplicate document id " << id << std::endl;
    }
}
