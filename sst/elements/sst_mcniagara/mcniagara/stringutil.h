
#ifndef STRINGUTIL_H
#define STRINGUTIL_H 
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
namespace StringUtil{
vector<string> &split_string(const string &s, char delim, vector<string> &elems);


vector<string> split_string(const string &s, char delim);

string stringToken(string *str,string aFormat);
}//end namespace std
#endif
