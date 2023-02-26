#include "search_server.h"
#include "request_queue.h"
#include "paginator_impl.h"
#include "read_input_functions.h"
#include "text_example_functions.h"

int main() {
    TestSearchServer();
    SearchServer search_server("и в на g"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL,
                              {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, {9});

    std::cout << "ACTUAL by default:"s << std::endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        std::cout << document << std::endl;
    }

    std::cout << "BANNED:"s << std::endl;
    for (const Document& document :
         search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        std::cout << document << std::endl;
    }

    std::cout << "Even ids:"s << std::endl;
    for (const Document &document :
         search_server.FindTopDocuments("пушистый ухоженный кот"s,
                                        [](int document_id, DocumentStatus status, int rating) {
                                            return document_id % 2 == 0;
                                        }))  //
    {
        std::cout << document << std::endl;
    }

    std::cout << std::endl << "Paginator:" << std::endl;

    const auto search_results = search_server.FindTopDocuments("кот скворец пёс"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Page break"s << std::endl;
    }

    std::cout << std::endl << "RequestQueue:" << std::endl;
    
    RequestQueue request_queue(search_server);

    auto evens_result = request_queue.AddFindRequest("пушистый ухоженный кот"s,
                                        [](int document_id, DocumentStatus status, int rating) {
                                            return document_id % 2 == 0;
                                        });
    for (const Document &document : evens_result){
        std::cout << document << std::endl;
    }

    for (auto id : search_server){
        std::cout << id << std::endl;
    }

    return 0;
}
