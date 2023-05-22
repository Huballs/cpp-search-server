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

The constructor accepts a string of stop-words, for example: "in at and"s
The stop-word in the query is not taken into account when searching.

Adding documents to the search engine. 
`void AddDocument(int document_id, string_view document,DocumentStatus status, const vector<int> &ratings);`

document - string as: "funny pet and nasty -rat"s
where "funny pet nasty" - the words that will be searched for
"and" - the stop word specified in the SearchServer constructor
"-rat" - mius-word
Mnius-words exclude documents containing such words from the search results.
Possible DocumentStatus: ACTUAL, IRRELEVANT, BANNED, REMOVED
ratings - Each document at the input has a set of user ratings.
The first digit is the number of ratings
For example:{4 5 -12 2 1};

Document search in the search server and ranking by TF-IDF
There are 6 ways to call the function 3 multithreaded (ExecutionPolicy) and 3 single-threaded
FindTopDocuments (ExecutionPolicy,query)
FindTopDocuments (ExecutionPolicy,query,DocumentStatus)
FindTopDocuments (ExecutionPolicy,query,DocumentPredicate)
FindTopDocuments (query)
FindTopDocuments (query,DocumentStatus)
FindTopDocuments (query,DocumentPredicate)

Returns vector matching by query
The usefulness of words is evaluated by the concept of inverse document frequency or IDF.
This value is a property of a word, not a document.
The more documents have a word in them, the lower the IDF.
Above, place documents where the search word occurs more than once.
Here you need to calculate the term frequency or TF.
For a specific word and a specific document, this is the share that this word occupies among all.

GetDocumentCount() - returns the number of documents in the search server

begin и end - They will return iterators. The iterator will give access to the id of all documents stored in the search server.

tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(raw_query, document_id)
Returns:The first object is a vector of query words that were found in the document_id document,
and the second object is the status of the document

Method of obtaining word frequencies by document id:
map<string, double> GetWordFrequencies(document_id)

Deleting a document from the search server
RemoveDocument(document_id)
RemoveDocument(ExecutionPolicy,document_id)
RemoveDocument(ExecutionPolicy, document_id)
