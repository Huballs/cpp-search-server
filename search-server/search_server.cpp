#include "search_server.h"

SearchServer::SearchServer(std::string_view stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text)){
}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text)){
}

std::tuple<std::vector<std::string_view>, DocumentStatus> 
SearchServer::MatchDocument(std::execution::parallel_policy policy, 
                std::string_view raw_query,
                int document_id) const {

    if(!id_list_.count(document_id))
        throw std::out_of_range("ID doesn't exist");

    const auto query = ParseQueryFast(raw_query);

    auto is_minus_word =
    std::find_if(policy, query.minus_words.begin(),
                query.minus_words.end(),
                [this, &document_id](const std::string_view word){
                    auto it = document_to_word_freqs_.find(document_id);
                    return it != document_to_word_freqs_.end() && it->second.count(word) != 0;
                }               
    );

    if(is_minus_word != query.minus_words.end())
        return {std::vector<std::string_view>{}, documents_.at(document_id).status};

    std::vector<std::string_view> matched_words(query.plus_words.size());

    std::copy_if(policy, query.plus_words.begin(),
                query.plus_words.end(),
                matched_words.begin(),
                [this, &document_id](const std::string_view word){
                    auto it = document_to_word_freqs_.find(document_id);
                    return it != document_to_word_freqs_.end() && it->second.count(word) != 0;
                }       
    );

    std::sort(policy, matched_words.begin(), matched_words.end());

    auto last_matched_word = std::unique(policy, matched_words.begin(), matched_words.end());
    /* Пустые строки(если plus_words > matched_words) у меня сортируются в начало 
        (в тренажере в конец (?)), надо удалить: */
    auto first_matched_word = 
    (*matched_words.begin() == "" ? next(matched_words.begin()) : matched_words.begin());
    
    return {{first_matched_word, last_matched_word}, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> 
SearchServer::MatchDocument(std::string_view raw_query,
                int document_id) const{

    if(!id_list_.count(document_id))
        throw std::out_of_range("ID doesn't exist");

    const auto query = ParseQuerySorted(raw_query);

    for (const std::string_view word : query.minus_words) {
        if (document_to_word_freqs_.at(document_id).count(word)) {
            return {std::vector<std::string_view>{}, documents_.at(document_id).status};
        }
    }

    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        auto it = word_to_document_freqs_.find(word);
        if(it != word_to_document_freqs_.end()){
            if(it->second.count(document_id)){
                matched_words.push_back(it->first);
            }
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> 
SearchServer::MatchDocument(std::execution::sequenced_policy policy, 
        std::string_view raw_query,
        int document_id) const{
            
        return MatchDocument(raw_query, document_id);
}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status,
                    const std::vector<int>& ratings) {
    if (document_id < 0)
        throw std::invalid_argument("Negative ID"s);
    if (documents_.find(document_id) != documents_.end())
        throw std::invalid_argument("ID already exists"s);
    if (!IsValidWord(document))
        throw std::invalid_argument("Text contains symbols that are not allowed"s);

    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        auto [it, _] = words_storage_.emplace(word);
        word_to_document_freqs_[*it][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][*it] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    id_list_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;});
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const{
    auto it = document_to_word_freqs_.find(document_id);
    if(it == document_to_word_freqs_.end()){
        static std::map<std::string_view, double> word_freqs_empty;
        return word_freqs_empty;
    }

    return it->second;
}

std::set<int>::const_iterator SearchServer::begin() const {
    return id_list_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return id_list_.end();
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

[[nodiscard]] bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= 0 && c < 32;
    });
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    // Word shouldn't be empty
    if (text[0] == '-') {
        is_minus = true;
        if ((text.size() == 1) || (text[1] == '-'))
            throw std::invalid_argument("Query contains invalid request"s);
        text = text.substr(1);
    }
    if (!IsValidWord(text))
        throw std::invalid_argument("Query contains symbols that are not allowed"s);

    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQueryFast(std::string_view text) const {
    std::vector<std::string_view> plus_words;
    std::vector<std::string_view> minus_words;

    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                minus_words.push_back(query_word.data);
            }
            else {
                plus_words.push_back(query_word.data);
            }
        }
    }

    return {plus_words, minus_words};
}

SearchServer::Query SearchServer::ParseQuerySorted(std::string_view text) const{
    Query result;

    std::set<std::string_view> plus_words;
    std::set<std::string_view> minus_words;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                minus_words.insert(query_word.data);
            }
            else {
                plus_words.insert(query_word.data);
            }
        }
    }

    return {std::vector<std::string_view>(plus_words.begin(), plus_words.end()),
            std::vector<std::string_view>(minus_words.begin(), minus_words.end())};
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}