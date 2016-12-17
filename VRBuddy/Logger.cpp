#include "Logger.h"
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <vector>
using namespace std;

	void Logger::log(string text, bool truncate, bool log_to_console)
	{
		if (truncate)
		{
			//truncate file and start anew
			std::ofstream log_file("log.txt", ios::out);

			//http://en.cppreference.com/w/c/chrono/ctime
			//Time and Date at top
			time_t time_data = time(NULL);	//Gets time data.
			char output[26];	//string to hold formatted time data.
			ctime_s(output, sizeof(output), &time_data);	//Populate string.

			log_file << "Time/Date of Log: " << output << endl << endl;	//output to file.

			log_file << text << endl;	//Output other text.
			log_file.close();
		}
		else
		{
			//append to file, not replace/trunicate
			std::ofstream log_file("log.txt", ios::app);
			log_file << text << endl;
			log_file.close();
		}

		if (log_to_console)
		{
			cout << text << endl;
		}
	}

	void Logger::log_multiple(vector<string> text, bool truncate, bool log_to_console)
	{
		if (truncate)
		{
			//truncate file and start anew
			std::ofstream log_file("log.txt", ios::out);

			//http://en.cppreference.com/w/c/chrono/ctime
			//Time and Date at top
			time_t time_data = time(NULL);	//Gets time data.
			char output[26];	//string to hold formatted time data.
			ctime_s(output, sizeof(output), &time_data);	//Populate string.

			log_file << "Time/Date of Log: " << output << endl << endl;	//output to file.

			for (int i = 0; i < text.size(); i++)
			{
				log_file << text[i] << endl;	//Output other text.
			}

			log_file.close();
		}
		else
		{
			//append to file, not replace/trunicate
			std::ofstream log_file("log.txt", ios::app);

			for (int i = 0; i < text.size(); i++)
			{
				log_file << text[i] << endl;



				if (log_to_console)
				{
					cout << text[i] << endl;
				}
			}
			log_file.close();
		}
	}
