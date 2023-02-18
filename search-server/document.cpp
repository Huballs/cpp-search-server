#include "document.h"

std::ostream& operator<<(std::ostream& os, const Document& doc) {
    using namespace std::string_literals;
    os << "{ "s
        << "document_id = "s << doc.id << ", "s
        << "relevance = "s << doc.relevance << ", "s
        << "rating = "s << doc.rating << " }"s;
    return os;
}
