# cpp-search-server
Финальный проект: поисковый сервер

# Description
The search server provides parsing of search queries, document search, sorting of results by specified criteria, removal of duplicates. Server operation is accelerated due to multithreading and use of the std::string_view class.

# Build Project using Cmake
```
mkdir Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

# Description of features:
The core of the search engine is the class: SearchServer
Methods of the SearchServer class:

The constructor accepts a string of stop-words, for example: "in at and"s.
The stop-word in the query is not taken into account when searching.

Adding documents to the search engine:
```
void AddDocument(int document_id, string_view document,DocumentStatus status, const vector<int> &ratings);
```
`document` - a string: "funny pet and nasty -rat"s
where "funny pet nasty" - the words that will be searched for.
"and" - the stop word specified in the SearchServer constructor
"-rat" - minus-word.
Minus-words exclude documents containing such words from the search results.

`enum DocumentStatus:`
```
ACTUAL, IRRELEVANT, BANNED, REMOVED
```
`ratings` - Each document at the input has a set of user ratings.
The first digit is the number of ratings;
For example:{4 5 -12 2 1};

Document search in the search server and ranking by TF-IDF
There are 6 ways to call the function: 3 multithreaded (ExecutionPolicy) and 3 single-threaded
```
FindTopDocuments (ExecutionPolicy,query)
FindTopDocuments (ExecutionPolicy,query,DocumentStatus)
FindTopDocuments (ExecutionPolicy,query,DocumentPredicate)
FindTopDocuments (query)
FindTopDocuments (query,DocumentStatus)
FindTopDocuments (query,DocumentPredicate)
```
They return vector of `document` type matching the query.
The usefulness of words is evaluated by the concept of inverse document frequency or IDF.
This value is a property of a word, not a document.
The more documents have a word in them, the lower the IDF.

```
GetDocumentCount()
``` 
 - returns the number of documents in the search server

```
begin
end
```
- return iterators. The iterator will give access to the id of all documents stored in the search server.
```
tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(raw_query, document_id)
```
- returns a vector of query words that were found in the document_id document,
and the second object is the status of the document

Method of obtaining word frequencies by document id:
```
map<string, double> GetWordFrequencies(document_id)
```

Deleting a document from the search server:
```
RemoveDocument(document_id)
RemoveDocument(ExecutionPolicy,document_id)
RemoveDocument(ExecutionPolicy, document_id)
```

# Technology stack:
- The project shows knowledge of the basic principles of C++ programming:
  - Numbers, strings, symbols, data input/output in console, conditions, loops.
  - The use of basic algorithms .
  - Using structures, classes, lambda functions, creating tuples
  - Parsign of lines with output to the screen
  - Use of Templates and Specialization of templates
  - Creating and using macros
  - Overloading operators
  - Handling exceptions
  - Application of the optional class
  - Iterators
  - Recursion
  - Stack, Dec
  - Creating and using macros
  - Overloading operators
  - Handling exceptions
  - Application of the optional class
  - Iterators
  - Working with standard input/output streams Working with standard input/output streams
  - Static, Automatic and Dynamic placement of objects in memory
  - Parallel work using the bible library
  - Scan algorithms
  - Asynchronous calculations using the library
  - Race state protection: mutex, lock_guard, atomic-types
- Calculation of term frequency and inverse document frequency
- Unit testing
- Decomposition and debugging
- Creating multi-file projects
- Profiling

- MapReduce as a concept in which the algorithm is divided into two stages:
independent filtering and transformation of individual elements (map or transform);
grouping of the results of the previous stage (reduce).
