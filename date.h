#ifndef __ODYS_DATE55__
#define __ODYS_DATE55__
#include <string>
#include <string.h>
#include <iostream>
using namespace std;

class Date {
private:
	int day, month, year;
public:
	Date(string date_string);
	Date(int day, int month, int year);
	int earlier_than(Date* other_date);
	int later_than(Date* other_date);
	int equal(Date* other_date);
	int still_in_hospital();
	void print();
	char* get_char_array();
	~Date();
};
#endif