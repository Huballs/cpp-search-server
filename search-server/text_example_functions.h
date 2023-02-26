#pragma once
#include <iostream>
#include "search_server.h"

std::ostream& operator<<(std::ostream& stream, DocumentStatus status);
std::ostream& operator<<(std::ostream& stream, std::vector<int>::const_iterator it);
template <typename F>
void RunTestImpl(const F& func, const std::string& name);
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint);
void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint);

#define RUN_TEST(func)  RunTestImpl(func, #func)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


void TestExcludeStopWordsFromAddedDocumentContent();
void TestFindAndAddDocument();
void TestMinusWords();
void TestSortByRelevancy();
void TestMatchDocument();
void TestRatings();
void TestMatchCustom();
void TestStatus();
void TestRelevancyCalculation();
void TestIdIterators();
void TestSearchServer();