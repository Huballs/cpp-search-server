#include "text_example_functions.h"


std::ostream& operator<<(std::ostream& stream, DocumentStatus status){
    stream << (int)status;
    return stream;
}

template <typename F>
void RunTestImpl(const F& func, const std::string& name) {
    func();
    std::cerr << name << " OK"s << std::endl;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsign-compare"
    if (t != u) {
    #pragma GCC diagnostic pop
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint) {
    if (!value) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server("that with the and this"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server("that with the and this"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("the"s).empty());
    }
}

void TestFindAndAddDocument(){
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    
    const int doc_id_2 = 55;
    const std::string content_2 = "dog in the house"s;
    
    {
    SearchServer server("that with the and this"s);
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
    SearchServer server("that with the and this"s);
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
    SearchServer server("that with the and this"s);
    const int doc_id_1 = 42;
    const std::string content_1 = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
    
    const int doc_id_2 = 55;
    const std::string content_2 = "dog in the house"s;

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
    SearchServer server("that"s);
    const int doc_id_1 = 42;
    const std::string content_1 = "gray cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 55;
    const std::string content_2 = "gray dog in the house"s;

    const int doc_id_3 = 1;
    const std::string content_3 = "red parrot in the bathhouse"s;

    const int doc_id_4 = 3;
    const std::string content_4 = "can't find me"s;
    

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
    SearchServer server("that with the and this"s);
    const int doc_id_1 = 42;
    const std::string content_1 = "blue cat in the city"s;
    const auto status_1 = DocumentStatus::ACTUAL;
    const std::vector<int> ratings = {1, 2, 3};

    const int doc_id_2 = 55;
    const auto status_2 = DocumentStatus::BANNED;
    const std::string content_2 = "gray dog in the house"s;

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
    SearchServer server("that with the and this"s);
    const int doc_id_1 = 42;
    const std::string content = "blue cat in the city"s;
    const auto status = DocumentStatus::ACTUAL;
    const std::vector<int> ratings_1 = {1};

    const int doc_id_2 = 55;
    const std::vector<int> ratings_2 = {1, 2, 3};

    const int doc_id_3 = 5;
    const std::vector<int> ratings_3 = {-1, 2, 3, 10};

    const int doc_id_4 = 6;
    const std::vector<int> ratings_4 = {-10, -5};

    const int doc_id_5 = 7;
    const std::vector<int> ratings_5 = {700, 0};

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
    SearchServer server("that with the and this"s);
    const int doc_id_1 = 1;
    const std::string content = "gray dog in the house"s;
    const auto status_1 = DocumentStatus::ACTUAL;
    const std::vector<int> ratings_1 = {1, 2, 3};

    const int doc_id_2 = 5;
    const auto status_2 = DocumentStatus::IRRELEVANT;
    const std::vector<int> ratings_2 = {10};

    const int doc_id_3 = 10;
    const auto status_3 = DocumentStatus::BANNED;
    const std::vector<int> ratings_3 = {100, 1};

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
    SearchServer server("that with the and this"s);
    const int doc_id_1 = 1;
    const std::string content = "gray dog in the house"s;
    const auto status_1 = DocumentStatus::ACTUAL;
    const std::vector<int> ratings = {1, 2, 3};

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
    SearchServer server("that with the and this"s);
    const auto status = DocumentStatus::ACTUAL;
    const auto rating = {1};

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
    std::cout << "Helllo" << std::endl;
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestFindAndAddDocument);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRatings);
    RUN_TEST(TestMatchCustom);
    RUN_TEST(TestStatus);
    RUN_TEST(TestRelevancyCalculation);
}