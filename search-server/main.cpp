#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double COMPARE_TOLERANCE = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
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
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

// ------- Test funcs and macros -------

ostream& operator<<(ostream& stream, DocumentStatus status){
    stream << (int)status;
    return stream;
}

template <typename F>
void RunTestImpl(const F& func, const string& name) {
    func();
    cerr << name << " OK"s << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

// ------- end of test funcs and macros -------

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename Match_f>
    vector<Document> FindTopDocuments(const string& raw_query, Match_f match_f) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, match_f);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < COMPARE_TOLERANCE) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    vector<Document> FindTopDocuments(const string& raw_query, 
                                    DocumentStatus doc_status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, 
        [doc_status](int document_id, DocumentStatus status, int rating) { return status == doc_status;});
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);

        vector<string> matched_words;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData 
    {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

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

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Match_f>
    vector<Document> FindAllDocuments(const Query& query, Match_f match_f) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& doc = documents_.at(document_id);
                if (match_f(document_id, doc.status, doc.rating)){
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestFindAndAddDocument(){
    const int doc_id_1 = 42;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    
    const int doc_id_2 = 55;
    const string content_2 = "dog in the house"s;
    
    {
    SearchServer server;
    // add one and find one document
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
    auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_1);

    // add another, find one
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
    found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_1);

    // find two
    found_docs = server.FindTopDocuments("in"s);
    ASSERT_EQUAL(found_docs.size(), 2);
    ASSERT_EQUAL(found_docs[0].id, doc_id_1);
    ASSERT_EQUAL(found_docs[1].id, doc_id_2);
    }

    {
    SearchServer server;
    // add 10, find top five
    for(int i = 0; i<10; ++i){
        server.AddDocument(i, "test string"s, DocumentStatus::ACTUAL, ratings);
    }
    auto found_docs = server.FindTopDocuments("test"s);
    ASSERT_EQUAL(found_docs.size(), 5);
    for(int i = 0; i<10; ++i){
        ASSERT_EQUAL(found_docs[i].id, i);
    }
    }

}

void TestMinusWords(){
    SearchServer server;
    const int doc_id_1 = 42;
    const string content_1 = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    
    const int doc_id_2 = 55;
    const string content_2 = "dog in the house"s;

    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);

    auto found_docs = server.FindTopDocuments("test -in"s);
    ASSERT_EQUAL(found_docs.size(), 0);

    found_docs = server.FindTopDocuments("dog -cat"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_2);

    found_docs = server.FindTopDocuments("in -cat"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_2);

    for(int i = 0; i<10; ++i){
        server.AddDocument(i, "test string"s, DocumentStatus::ACTUAL, ratings);
    }
    found_docs = server.FindTopDocuments("in -test"s);
    ASSERT_EQUAL(found_docs.size(), 2);
    ASSERT_EQUAL(found_docs[0].id, doc_id_1);
    ASSERT_EQUAL(found_docs[1].id, doc_id_2);

    found_docs = server.FindTopDocuments("in -cat -test"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_2);

}

void TestSortByRelevancy(){
    SearchServer server;
    const int doc_id_1 = 42;
    const string content_1 = "gray cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 55;
    const string content_2 = "gray dog in the house"s;

    const int doc_id_3 = 1;
    const string content_3 = "red parrot in the bathhouse"s;

    const int doc_id_4 = 3;
    const string content_4 = "can't find me"s;
    

    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id_4, content_4, DocumentStatus::ACTUAL, ratings);
    
    const auto found_docs = server.FindTopDocuments("gray dog in the car"s);

    ASSERT_EQUAL(found_docs.size(), 3);

    ASSERT_EQUAL(found_docs[0].id, doc_id_2);
    ASSERT_EQUAL(found_docs[1].id, doc_id_1);
    ASSERT_EQUAL(found_docs[2].id, doc_id_3);

}

void TestMatchDocument(){
    SearchServer server;
    const int doc_id_1 = 42;
    const string content_1 = "blue cat in the city"s;
    const auto status_1 = DocumentStatus::ACTUAL;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 55;
    const auto status_2 = DocumentStatus::BANNED;
    const string content_2 = "gray dog in the house"s;

    server.AddDocument(doc_id_1, content_1, status_1, ratings);
    server.AddDocument(doc_id_2, content_2, status_2, ratings);
    {
    auto [matched_words, status] = server.MatchDocument("in gray dog and white parrot"s, doc_id_2);
    ASSERT_EQUAL(matched_words.size(), 3);
    ASSERT_EQUAL(status, status_2);
    ASSERT_EQUAL(matched_words[0], "dog"s);
    ASSERT_EQUAL(matched_words[1], "gray"s);
    ASSERT_EQUAL(matched_words[2], "in"s);
    }
    {
    auto [matched_words, status] = server.MatchDocument("in gray dog and white parrot"s, doc_id_1);
    ASSERT_EQUAL(status, status_1);
    ASSERT_EQUAL(matched_words.size(), 1);
    ASSERT_EQUAL(matched_words[0], "in"s);
    }
    {
    {
    auto [matched_words, status] = server.MatchDocument("mouse"s, doc_id_1);
    ASSERT_EQUAL(matched_words.size(), 0);
    ASSERT_EQUAL(status, status_1);
    }
    {
    auto [matched_words, status] = server.MatchDocument("in gray dog and white parrot -house"s, doc_id_2);
    ASSERT_EQUAL(matched_words.size(), 0);
    ASSERT_EQUAL(status, status_2);
    }
    }
    
}

void TestRatings(){
    SearchServer server;
    const int doc_id_1 = 42;
    const string content = "blue cat in the city"s;
    const auto status = DocumentStatus::ACTUAL;
    const vector<int> ratings_1 = {1};

    const int doc_id_2 = 55;
    const vector<int> ratings_2 = {1, 2, 3};

    const int doc_id_3 = 5;
    const vector<int> ratings_3 = {-1, 2, 3, 10};

    const int doc_id_4 = 6;
    const vector<int> ratings_4 = {-10, -5};

    const int doc_id_5 = 7;
    const vector<int> ratings_5 = {700, 0};

    server.AddDocument(doc_id_1, content, status, ratings_1);
    server.AddDocument(doc_id_2, content, status, ratings_2);
    server.AddDocument(doc_id_3, content, status, ratings_3);
    server.AddDocument(doc_id_4, content, status, ratings_4);
    server.AddDocument(doc_id_5, content, status, ratings_5);

    auto found_docs = server.FindTopDocuments(content, status);
    
    ASSERT_EQUAL(found_docs[0].rating, (accumulate(ratings_5.begin(), ratings_5.end(), 0)/(int)ratings_5.size()));
    ASSERT_EQUAL(found_docs[1].rating, (accumulate(ratings_3.begin(), ratings_3.end(), 0)/(int)ratings_3.size()));
    ASSERT_EQUAL(found_docs[2].rating, (accumulate(ratings_2.begin(), ratings_2.end(), 0)/(int)ratings_2.size()));
    ASSERT_EQUAL(found_docs[3].rating, (accumulate(ratings_1.begin(), ratings_1.end(), 0)/(int)ratings_1.size()));
    ASSERT_EQUAL(found_docs[4].rating, (accumulate(ratings_4.begin(), ratings_4.end(), 0)/(int)ratings_4.size()));
}

void TestMatchCustom(){
    SearchServer server;
    const int doc_id_1 = 1;
    const string content = "gray dog in the house"s;
    const auto status_1 = DocumentStatus::ACTUAL;
    const vector<int> ratings_1 = {1, 2, 3};

    const int doc_id_2 = 5;
    const auto status_2 = DocumentStatus::IRRELEVANT;
    const vector<int> ratings_2 = {10};

    const int doc_id_3 = 10;
    const auto status_3 = DocumentStatus::BANNED;
    const vector<int> ratings_3 = {100, 1};

    server.AddDocument(doc_id_1, content, status_1, ratings_1);
    server.AddDocument(doc_id_2, content, status_2, ratings_2);
    server.AddDocument(doc_id_3, content, status_3, ratings_3);

    auto found_docs = server.FindTopDocuments("dog"s, 
        [doc_id_1](const auto id, const auto status, const auto rating){ return id == doc_id_1;});
    
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_1);
    
    found_docs = server.FindTopDocuments("dog"s, 
        [doc_id_1](const auto id, const auto status, const auto rating){ return status == DocumentStatus::BANNED;});
    
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_3);
    
    found_docs = server.FindTopDocuments("dog"s, 
        [doc_id_1](const auto id, const auto status, const auto rating){ return rating == 10;});
    
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_2);
}

void TestStatus(){
    SearchServer server;
    const int doc_id_1 = 1;
    const string content = "gray dog in the house"s;
    const auto status_1 = DocumentStatus::ACTUAL;
    const vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 5;
    const auto status_2 = DocumentStatus::IRRELEVANT;


    const int doc_id_3 = 10;
    const auto status_3 = DocumentStatus::BANNED;

    const int doc_id_4 = 15;
    const auto status_4 = DocumentStatus::REMOVED;

    const int doc_id_5 = 16;
    const auto status_5 = DocumentStatus::REMOVED;

    server.AddDocument(doc_id_1, content, status_1, ratings);
    server.AddDocument(doc_id_2, content, status_2, ratings);
    server.AddDocument(doc_id_3, content, status_3, ratings);
    server.AddDocument(doc_id_4, content, status_4, ratings);
    server.AddDocument(doc_id_5, content, status_5, ratings);

    auto found_docs = server.FindTopDocuments("dog"s, status_1);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_1);

    found_docs = server.FindTopDocuments("dog"s, status_2);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_2);

    found_docs = server.FindTopDocuments("dog"s, status_3);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id_3);

    found_docs = server.FindTopDocuments("dog"s, status_4);
    ASSERT_EQUAL(found_docs.size(), 2);
    ASSERT_EQUAL(found_docs[0].id, doc_id_4);
    ASSERT_EQUAL(found_docs[1].id, doc_id_5);
}

void TestRelevancyCalculation(){
    SearchServer server;
    const auto status = DocumentStatus::ACTUAL;
    const auto rating = {1};
    server.SetStopWords("that with the and this"s);

    server.AddDocument(0, "gray dog"s, status, rating);
    server.AddDocument(1, "pretty cat with gray tail"s, status, rating);
    server.AddDocument(2, "our cat ran away with the neighbours dog"s, status, rating);
    server.AddDocument(3, "this dog is not mine"s, status, rating);
    server.AddDocument(4, "this crazy dog bit my other dog and now its gray very gray"s, status, rating);

    auto found_docs = server.FindTopDocuments("gray dog"s);

    ASSERT_EQUAL(found_docs.size(), 5);

    auto comp = [](double a, double b){ 
        if (abs(a - b) < COMPARE_TOLERANCE) return true;
        return false;
    ;};

    ASSERT(comp(found_docs[0].relevance, 0.366985)); // doc_id_0
    ASSERT(comp(found_docs[1].relevance, 0.133449)); // doc_id_4
    ASSERT(comp(found_docs[2].relevance, 0.127706)); // doc_id_1
    ASSERT(comp(found_docs[3].relevance, 0.0557859)); // doc_id_3
    ASSERT(comp(found_docs[4].relevance, 0.0371906)); // doc_id_2
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestFindAndAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRatings);
    RUN_TEST(TestMatchCustom);
    RUN_TEST(TestStatus);
    RUN_TEST(TestRelevancyCalculation);
}

// --------- Окончание модульных тестов поисковой системы -----------

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

int main() {
    TestSearchServer();

    SearchServer search_server;
    search_server.SetStopWords("that with the and this"s);

    search_server.AddDocument(0, "gray dog"s,       DocumentStatus::ACTUAL, {50});
    search_server.AddDocument(1, "pretty cat with gray tail"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "our cat ran away with the neighbours dog"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "this dog is not mine"s,         DocumentStatus::ACTUAL, {9});
    search_server.AddDocument(4, "this crazy dog bit my other dog and now its gray very gray"s,         DocumentStatus::IRRELEVANT, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("gray dog"s)) {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("gray dog"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; })) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("gray dog"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}