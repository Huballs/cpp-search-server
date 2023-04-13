#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text)){  // Invoke delegating constructor from string container
}

std::tuple<std::vector<std::string>, DocumentStatus> 
SearchServer::MatchDocument(std::execution::parallel_policy policy, 
                const std::string& raw_query,
                int document_id) const{

    if(!id_list_.count(document_id))
        throw std::out_of_range("ID doesn't exist");

    const auto query = ParseQuery(raw_query);   
/*
    std::vector<std::string> plus_words(
                            query.plus_words.begin(),//std::make_move_iterator(query.plus_words.begin()),
                            query.plus_words.end()//std::make_move_iterator(query.plus_words.end())
                         );
    std::vector<std::string> minus_words(
                            query.minus_words.begin(),//std::make_move_iterator(query.minus_words.begin()),
                            query.minus_words.end()//std::make_move_iterator(query.minus_words.end())
                         );
*/
    auto if_minus_word =
    std::any_of(policy, query.minus_words.begin(),
                query.minus_words.end(),
                [this, &document_id](const std::string& word){
                    return document_to_word_freqs_.at(document_id).count(word);
                }              
    );
    //if(it_minus_word != minus_words.end())
    if(if_minus_word)
        return {std::vector<std::string>{}, documents_.at(document_id).status};

    std::vector<std::string> matched_words(query.plus_words.size());

    auto it_matched_max =
    std::copy_if(policy, query.plus_words.begin(),
                query.plus_words.end(),
                matched_words.begin(),
                [this, &document_id](const std::string& word){
                    return document_to_word_freqs_.at(document_id).count(word);
                }
    );
    matched_words.resize(it_matched_max - matched_words.begin());

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string>, DocumentStatus> 
SearchServer::MatchDocument(const std::string& raw_query,
                int document_id) const{

    if(!id_list_.count(document_id))
        throw std::out_of_range("ID doesn't exist");

    const auto query = ParseQuery(raw_query);

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            //matched_words.clear();
            //break;
            return {std::vector<std::string>{}, documents_.at(document_id).status};
        }
    }

    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                    const std::vector<int>& ratings) {
    if (document_id < 0)
        throw std::invalid_argument("Negative ID"s);
    if (documents_.find(document_id) != documents_.end())
        throw std::invalid_argument("ID already exists"s);
    if (!IsValidWord(document))
        throw std::invalid_argument("Text contains symbols that are not allowed"s);

    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    id_list_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const{
    auto it = document_to_word_freqs_.find(document_id);
    if(it == document_to_word_freqs_.end()){
        static std::map<std::string, double> word_freqs_empty;
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

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
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

[[nodiscard]] bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= 0 && c < 32;
    });
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
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

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    
    for (const std::string& word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}