#pragma once
#include "string_processing.h"
#include "log_duration.h"
#include "document.h"
#include <stdexcept>
#include <map>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <execution>
#include <string_view>

using namespace std::string_literals;

const double COMPARE_TOLERANCE = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    explicit SearchServer(const std::string_view stop_words_text);
    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const char* stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(std::execution::parallel_policy policy, 
                  const std::string_view raw_query,
                  int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(std::execution::sequenced_policy policy, 
                  const std::string_view raw_query,
                  int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> 
    MatchDocument(const std::string_view raw_query,
                  int document_id) const;

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    template <class ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);

    void RemoveDocument(int document_id){
        RemoveDocument(std::execution::seq, document_id);
    }

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    struct QueryPar {
        std::vector<std::string> plus_words;
        std::vector<std::string> minus_words;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> id_list_;

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view text) const;
    [[nodiscard]] static bool IsValidWord(const std::string_view word);

    static int ComputeAverageRating(const std::vector<int>& ratings);

    QueryWord ParseQueryWord(std::string text) const;

    SearchServer::Query ParseQuery(const std::string_view text) const;
    SearchServer::QueryPar ParseQueryParallel(const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const;
};


// ------------------------------------------------- TEMPLATE FUNCTIONS -----------------------------------------------------//
template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        :stop_words_(MakeUniqueNonEmptyStrings(stop_words)){
            
    for (const auto& word : stop_words_){
        if(!IsValidWord(word))
            throw std::invalid_argument("Invalid word: "s + word);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
                                    DocumentPredicate document_predicate) const {
    
    const auto query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < COMPARE_TOLERANCE) {
                    return lhs.rating > rhs.rating;
                } else {
                    return lhs.relevance > rhs.relevance;
                }
            });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return  matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                    DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <class ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id){
    id_list_.erase(document_id);   

    std::vector<const std::string*> words(document_to_word_freqs_[document_id].size());

    std::transform(policy,
        document_to_word_freqs_[document_id].begin(),
        document_to_word_freqs_[document_id].end(),
        words.begin(),
        [](const auto &word){
            return &(word.first);
        }
    );

    std::for_each(policy, words.begin(), words.end(),
        [this, document_id](const std::string* word) {
            word_to_document_freqs_[*word].erase(document_id);
        }
    );
        
    document_to_word_freqs_.erase(document_id);

    documents_.erase(document_id);   
}