#pragma once
#include <iostream>
#include <execution>
#include "search_server.h"
#include "paginator_impl.h"
#include "remove_duplicates.h"
#include "process_queries.h"
#include <random>
#include <string_view>

std::ostream& operator<<(std::ostream& stream, DocumentStatus status);
std::ostream& operator<<(std::ostream& stream, std::set<int>::const_iterator it);
std::ostream& operator<<(std::ostream& stream, std::map<std::string, double> word_freqs);

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
void TestMatchDocumentMine();
void TestRatings();
void TestMatchCustom();
void TestStatus();
void TestRelevancyCalculation();
void TestIdIterators();
void TestWordFreqs();
void TestPaginator();
void TestRemoveDocument();
void TestProcessQueriesMine();

void TestSearchServer();