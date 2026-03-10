#include<iostream>
#include<fstream>
#include<unordered_map>
#include<sstream>
#include<string>
#include<vector>
#include<algorithm>
#include<chrono>
using namespace std;

/* Steps:
1. From our code, load a file to read
--load multiple files - versions
2. Fixed Size Buffer - Read the file incrementaly in fixed sized chunks
--edge cases - splitting of words between two different chunks
3. store all the words in a list along with their frequencies (unordered_map)
4.Search queries(3`)
*/

/**********************************
 CLASS_1 - BUFFERED READER
 **********************************/

class buffer_reader{
private:
	ifstream file; 		//this connects the file in the memory/disk to our source code
	char* buffer; 		//this is the buffer : an array of bytes (since 1 byte = char), array of chars
	int buffer_size; 	//this is how many bytes (= size of array = size of buffer)
	int bytes_read; 	// how many bytes are read from the file into the buffer
public:

	/*-------------------------------
	function 1: runs when an object of this class is created, takes the filename and buffer size in KB
	---------------------------------*/
	buffer_reader(string filename, int buffersize_kb){
		buffer_size = buffersize_kb*1024; 				//conversion : kb ---> bytes
		buffer = new char[buffer_size]; 				//allocate the buffer inside RAM at runtime
		bytes_read = 0; 								//current read = 0
		file.open(filename); 							//opent the connection

		//if file doesnt exist or cant be opened ---> throw error
		if(!file.is_open()){
			throw runtime_error("Cannot open the file: " + filename);
		}
	}
	
	/*-------------------------------
	function 2: destructor - runs when the object is destroyed - free everything we allocated
	---------------------------------*/
	~buffer_reader(){
		if(file.is_open()) file.close(); 		//close connection
		delete[] buffer; 						//free RAM
		buffer = nullptr; 						//for safety - null the pointer
	}
	
	/*-------------------------------
	function 3: reads next chunk from file into the buffer - true: we get bytes read - false: everything read, loop stops
	---------------------------------*/
	bool read_next_chunk(){
		file.read(buffer, buffer_size); 	//pull from disk to buffer
		bytes_read = file.gcount(); 		//how many bytes came in
		return bytes_read > 0; 				//false when nothing left to read
	}

	//give access to buffer contents
	char* get_buffer() { return buffer; } 

	//tell how many bytes to process
	int get_bytes_read() { return bytes_read; }

};

/**********************************
 CLASS_2 - TOKENIZER
 **********************************/

class tokenizer{
private:
	string leftover; //stores incomplete word from previous buffer
public:

	/*-------------------------------
	function 1: constructor - leftover starts empty
	---------------------------------*/
	tokenizer(){
		leftover = "";
	}

	/*-------------------------------
	function 2: conversion into the items of a list of words
	---------------------------------*/
	vector<string> tokenize(char* buffer, int bytes_read){
		vector<string> words;
		string current = leftover;
		leftover = "";
		for(int i=0; i< bytes_read; i++){
			char c = buffer[i];
			if(isalnum(c)){
				current += tolower(c);
			}
			else{
				if(!current.empty()){
					words.push_back(current);
					current = "";
				}
			}
		}
		leftover = current;
		return words;
	}

	/*-------------------------------
	function 3: called after the last buffer to flush any remaining words
	---------------------------------*/
	vector<string> tokenize(){
		vector<string> words;
		if(!leftover.empty()){
			words.push_back(leftover);
			leftover = "";
		}
		return words;
	}
	
};

/**********************************
 CLASS_3 - VERSION INDEX
 **********************************/

class version_index{
private:
	unordered_map<string, unordered_map<string, int>> versions; //map of maps
public:

	void add_word(string version, string word){
		versions[version][word]++;
	}

	/*-------------------------------
	function overloading
	---------------------------------*/

	//get count of a word given "version + word"
	int get_count(string version, string word){
		if(versions.find(version) == versions.end()){
			throw runtime_error("Version not found: " + version);
		}
		if(versions[version].find(word) == versions[version].end()){
			return 0;
		}
		return versions[version][word];
	}

	//get count of a word in a specific version given the word
	int get_count(string word){
		int total = 0;
		for(auto& pair : versions){
			if(pair.second.find(word) != pair.second.end()){
				total += pair.second[word];
			}
		}
		return total;
	}

	/*-------------------------------
	function to get the entire map of the given version
	---------------------------------*/
	unordered_map<string, int>& get_index(string version) {
		if(versions.find(version) == versions.end()){
			throw runtime_error("Version not found: " + version);
		}
		return versions[version];
	}

	/*-------------------------------
	function to check if a version exists or not
	---------------------------------*/
	bool version_exists(string version){
		return versions.find(version) != versions.end();
	}
};

/**********************************
 TEMPLATE - get_top_k
 **********************************/
template <typename T>
vector<pair<T, int>> get_top_k(vector<pair<T, int>> items, int k) {

	/*job - returns top K items from a vector of pairs
 			T = key type can be (string, int, etc)
 			tie-breaking rule: alphabetical order for equal frequencies*/
    
    // sort by frequency descending, if frequencies are equal → sort by key ascending (alphabetical)
    sort(items.begin(), items.end(),
        [](const pair<T,int>& a, const pair<T,int>& b) {
            if (a.second != b.second) {
                return a.second > b.second;  // higher frequency first
            }
            return a.first < b.first;        // tie → alphabetical order
        });

    // cap at k or total size whichever is smaller
    int limit = min(k, (int)items.size());
    return vector<pair<T,int>>(items.begin(), items.begin() + limit);
}

/**********************************
 CLASS_4_to_7 - QUERIES
 **********************************/

class query_base{
public:
	//pure virtual - every derived class MUST implement this
	//this makes query_base abstract
	virtual void execute() = 0;
	//destructor
	virtual ~query_base(){}
};

class word_query : public query_base{
private:
	version_index& index;
	string version;
	string word;
public:

	word_query(version_index& idx, string ver, string w)
		: index(idx), version(ver), word(w) {}
	
	void execute() override{
		//get count
		int count = index.get_count(version, word);

		//print result
		cout << "\nVersion : " << version << endl;
		cout << "Query : word count of '" << word << "'" << endl;
		cout << "Result : " << count << endl;
	}
};

class topk_query : public query_base{
private:
	version_index& index;
	string version;
	int k;
public:

	topk_query(version_index& idx, string ver, int k)
		: index(idx), version(ver), k(k) {}

	void execute() override {
		//take the entire word map for this version
		unordered_map<string, int>& word_map = index.get_index(version);

		//copy into a vector for sorting
		vector<pair<string, int>> word_list(word_map.begin(), word_map.end());

		//using the top-k teplate
		vector<pair<string, int>> top = get_top_k(word_list, k);
		
		//print result
		cout << "\nVersion  : " << version << endl;
        cout << "Query    : top " << k << " words" << endl;
        cout << "Result   :" << endl;
		for(int i=0; i< (int)top.size(); i++){
			cout << " " << i+ 1 << ". "
				<< top[i].first
				<< "-->"
				<< top[i]. second
				<< endl;
		}
	}

};

class diff_query : public query_base{
private:
	version_index& index;
	string version1, version2;
	string word;
public:

	diff_query(version_index& idx, string v1, string v2, string w)
		: index(idx), version1(v1), version2(v2), word(w) {}

	void execute() override {
		int count1 = index.get_count(version1, word);
		int count2 = index.get_count(version2, word);
		int diff = count2 - count1;

		cout << "\nVersions : " << version1 << ", " << version2 << endl;
        cout << "Query    : diff for '" << word << "'" << endl;
        cout << "Result   : "
			<< (diff >=0 ? "+" : "")
			<< diff
			<< " (" << version1 << "=" << count1
			<< ", " << version2 << "=" << count2 << ")"
			<< endl;
	}

};

/**********************************
 MAIN_FUNCTION
 **********************************/

int main(int argc, char* argv[]) {

    // variables to store command line arguments
    string file1 = "", file2 = "";
    string version1 = "", version2 = "";
    string query_type = "";
    string word = "";
    int buffer_kb = 512;     // default buffer size
    int top_k = 0;

    // step_1: parse command line arguments
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if      (arg == "--file")     file1      = argv[++i];
        else if (arg == "--file1")    file1      = argv[++i];
        else if (arg == "--file2")    file2      = argv[++i];
        else if (arg == "--version")  version1   = argv[++i];
        else if (arg == "--version1") version1   = argv[++i];
        else if (arg == "--version2") version2   = argv[++i];
        else if (arg == "--query")    query_type  = argv[++i];
        else if (arg == "--word")     word       = argv[++i];
        else if (arg == "--buffer")   buffer_kb   = stoi(argv[++i]);
        else if (arg == "--top")      top_k       = stoi(argv[++i]);
    }

    // step_2: validating the buffer size
    try {
        if (buffer_kb < 256 || buffer_kb > 1024) {
            throw runtime_error("Buffer size must be between 256 and 1024 KB");
        }
    } catch (runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // step_3: start timing
    auto start_time = chrono::high_resolution_clock::now();

    // step_4: create index and tokenizer
    version_index index;
    tokenizer tokenizer1;

    // step_5: process file(s)
    try {
			// always process file1 with version1
			{
				buffer_reader reader(file1, buffer_kb);
				while (reader.read_next_chunk()) {
					// tokenize current chunk
					vector<string> words = tokenizer1.tokenize(
						reader.get_buffer(),
						reader.get_bytes_read()
					);
					// add each word to index
					for (string& w : words) {
						index.add_word(version1, w);
					}
				}
				// flush any leftover word at end of file
				vector<string> remaining = tokenizer1.tokenize();
				for (string& w : remaining) {
					index.add_word(version1, w);
				}
			}

			// if diff query → process file2 with version2
			if (query_type == "diff") {
				tokenizer tokenizer2;
				buffer_reader reader2(file2, buffer_kb);
				while (reader2.read_next_chunk()) {
					vector<string> words = tokenizer2.tokenize(
						reader2.get_buffer(),
						reader2.get_bytes_read()
					);
					for (string& w : words) {
						index.add_word(version2, w);
					}
				}
				vector<string> remaining = tokenizer2.tokenize();
				for (string& w : remaining) {
					index.add_word(version2, w);
				}
			}

    } catch (runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // step_6: run the right query
    try {
        query_base* query = nullptr;

        if (query_type == "word") {
            query = new word_query(index, version1, word);
        }
        else if (query_type == "top") {
            query = new topk_query(index, version1, top_k);
        }
        else if (query_type == "diff") {
            query = new diff_query(index, version1, version2, word);
        }
        else {
            throw runtime_error("Unknown query type: " + query_type);
        }

        query->execute();   // polymorphism — calls correct execute()
        delete query;       // free memory

    } catch (runtime_error& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    // step_7: print buffer size + final timing
    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = endTime - start_time;

    cout << "Buffer   : " << buffer_kb << " KB" << endl;
    cout << "Time     : " << elapsed.count() << " seconds" << endl;

    return 0;
}
