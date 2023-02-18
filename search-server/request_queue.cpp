#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) {
    search_server_ = &search_server;
    empty_result_count_ = 0;
}

int RequestQueue::GetNoResultRequests() const {       
    return empty_result_count_;
}

void RequestQueue::putInQueue(const std::string& raw_query, const std::vector<Document>& result) {
    if(requests_.size() >= min_in_day_){
        if(requests_.front().result.empty())
            --empty_result_count_;
        requests_.pop_front();
    }
    requests_.emplace_back(raw_query, result);
    if(result.empty()) 
        ++empty_result_count_;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> result = search_server_->FindTopDocuments(raw_query, status);
    putInQueue(raw_query, result);
    return result;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> result = search_server_->FindTopDocuments(raw_query);
    putInQueue(raw_query, result);
    return result;
}