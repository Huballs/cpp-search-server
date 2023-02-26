#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server){
    std::map<int, std::vector<std::string>> docs_to_words;
    std::vector<int> ids;

    for(auto& id : search_server){
        ids.push_back(id);
    }

    for(auto& id : ids){
        auto word_freq = search_server.GetWordFrequencies(id);
        for(auto& [word, _] : word_freq){
            // restore document
            docs_to_words[id].push_back(word);
        }
        // compare to previous documents
        int id_to_remove = -1;
        for(auto& [id_prev, words] : docs_to_words){
            if(id_prev != id){
                if(words == docs_to_words[id]){
                    id_to_remove = std::max(id, id_prev);
                    search_server.RemoveDocument(id_to_remove);
                    
                    std::cout << "Found duplicate document id " << id_to_remove << std::endl;
                }
            }
        }
        docs_to_words.erase(id_to_remove);
    }

}