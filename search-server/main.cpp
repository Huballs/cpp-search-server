#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();

        ++document_count_;

        for (const string& word : words){
            word_to_documents_[word][document_id] += inv_word_count; 
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        QueryContent query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        
        return matched_documents;
    }

private:
    struct QueryContent {
        set<string> words;
        set<string> exept_words;
    };

    //map<string, set<int>> word_to_documents_;   // {word, {ids where word can be found}}
    map<string, map<int, double>> word_to_documents_;   // {word, {ids where word can be found, TF}}
    set<string> stop_words_;                    // words that remove doc from result
    int document_count_ = 0;                    // total N of docs


    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    QueryContent ParseQuery(const string& text) const {
        QueryContent query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            
            if (word[0] == '-') {
                query_words.exept_words.insert((word.substr(1)));
            } else {
                query_words.words.insert(word);
            }
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const QueryContent& query_words) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance; // {id, relevance}
        //map<string, double> query_idf;  // {query, IDF}
        
        for (const auto& query_word : query_words.words) {
            if( auto it = word_to_documents_.find(query_word); it != word_to_documents_.end()){

                double  query_idf = log(static_cast <double> (document_count_)/it->second.size());                           

                for (auto &id : it->second){
                    //cout << id.first << " " << query_word << ": " << query_idf << " * " << id.second << endl;
                    document_to_relevance[id.first] += query_idf * id.second;
                }
            }
        }

        for(const auto& word : query_words.exept_words){
            if( auto it = word_to_documents_.find(word); it != word_to_documents_.end()){
                for (auto &id : it->second){
                    document_to_relevance.erase(id.first);
                }
            }
        }

        for(const auto& i : document_to_relevance){
            matched_documents.push_back({i.first, i.second});
        }
/*
        for (auto i : word_to_documents_){
            cout << i.first << " - ";
            for(auto j : i.second){
                cout << j.first << ":" << j.second << ", ";
            }
        }
        cout << endl;*/

        return matched_documents;
    }

};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}