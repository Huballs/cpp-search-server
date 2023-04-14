#pragma once
#include "search_server.h"
#include <queue>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;

private:
    void putInQueue(const std::string& raw_query, const std::vector<Document>& result);

    struct QueryResult {
        QueryResult(const std::string raw_query, std::vector<Document> result) 
            : raw_query_(raw_query), result_(result) {}

        const std::string raw_query_;
        std::vector<Document> result_;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer* search_server_;
    int empty_result_count_;
}; 

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> result = search_server_->FindTopDocuments(raw_query, document_predicate);
    putInQueue(raw_query, result);
    return result;
}
