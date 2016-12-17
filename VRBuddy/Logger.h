using namespace std;
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <vector>

class Logger
{
public:
	static void log(string text, bool truncate, bool log_to_console);
	static void log_multiple(vector<string> text, bool truncate, bool log_to_console);
};
