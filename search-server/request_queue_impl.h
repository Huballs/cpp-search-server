#pragma once
#include "search_server_impl.h"
#include <queue>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) {
        search_server_ = &search_server;
        empty_result_count_ = 0;
    }
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> result = search_server_->FindTopDocuments(raw_query, document_predicate);
        putInQueue(raw_query, result);
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status) {
        std::vector<Document> result = search_server_->FindTopDocuments(raw_query, status);
        putInQueue(raw_query, result);
        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query) {
        std::vector<Document> result = search_server_->FindTopDocuments(raw_query);
        putInQueue(raw_query, result);
        return result;
    }
    int GetNoResultRequests() const {       
        return empty_result_count_;
    }
private:
    void putInQueue(const std::string& raw_query, const std::vector<Document>& result) {
        if(requests_.size() >= min_in_day_){
            if(requests_.front().result.empty())
                --empty_result_count_;
            requests_.pop_front();
        }
        requests_.emplace_back(raw_query, result);
        if(result.empty()) 
            ++empty_result_count_;
    }

    struct QueryResult {
        QueryResult(const std::string raw_query, std::vector<Document> result) 
            : raw_query(raw_query), result(result) {}

        const std::string raw_query;
        std::vector<Document> result;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer* search_server_;
    int empty_result_count_;
}; 