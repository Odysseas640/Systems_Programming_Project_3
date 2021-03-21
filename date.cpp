#include "date.h"

Date::Date(int day, int month, int year) {
	this->day = day;
	this->month = month;
	this->year = year;
}
int is_date_string_OK(string date_string) { // Check whether this string is a valid date to avoid a segmentation fault
	// cout << "is_date_string_OK: " << date_string << endl;
	// cout << "length: " << date_string.length() << endl;
	if (date_string == "") {
		return 0;
	}
	if (date_string.length() < 5) {
		return 0;
	}
	int n_dashes = 0;
	if (date_string[0] < '0' || date_string[0] > '9') {
		return 0;
	}
	// if (date_string[date_string.length()-2] < '0' || date_string[date_string.length()-2] > '9') {
	// 	cout << "   not ok 2" << endl;
	// 	return 0;
	// }
	for (int i = 1; i < (int)date_string.length() - 1; ++i) {
		if ('0' <= date_string[i] && date_string[i] <= '9')
			continue;
		else if (date_string[i] == '-') {
			n_dashes++;
			if (date_string[i+1] < '0' || date_string[i+1] > '9') {
				return 0;
			}
			if (date_string[i-1] < '0' || date_string[i-1] > '9') {
				return 0;
			}
			if (n_dashes > 2) {
				return 0;
			}
		}
		else {
			return 0;
		}
	}
	if (n_dashes != 2) {
		return 0;
	}
	// cout << "Date String function OK\n";
	return 1;
}
Date::Date(string date_string) {
	char date_char_array[date_string.length() + 1];        // Char array
	for (int i = 0; i < (int)date_string.length(); ++i) { // Copy string to char array
		date_char_array[i] = date_string[i];
	}
	date_char_array[date_string.length()] = '\0';          // Make sure it ends with a \0
	char* saveptr = date_char_array;
	this->day = atoi(strtok_r(saveptr, "-", &saveptr));
	this->month = atoi(strtok_r(saveptr, "-", &saveptr)); // If month begins with a 0, it's still a decimal number, not octal. I checked.
	this->year = atoi(strtok_r(saveptr, "-", &saveptr));
	// cout << "DATE CONSTRUCTOR OK - day: " << this->day << ", month: " << this->month << ", year: " << this->year << endl;
}
int Date::earlier_than(Date* other_date) { // If it's equal it's not earlier
	if (this->day < 0 || this->month < 0) // Negative means it's an exit date and patient is still in hospital
		return 0;
	if (this->year < other_date->year)
		return 1;
	if (this->year == other_date->year && this->month < other_date->month)
		return 1;
	if (this->year == other_date->year && this->month == other_date->month && this->day < other_date->day)
		return 1;
	return 0;
}
int Date::later_than(Date* other_date) {
	if (this->day < 0 || this->month < 0) // Negative means it's an exit date and patient is still in hospital
		return 0;
	if (this->year > other_date->year)
		return 1;
	if (this->year == other_date->year && this->month > other_date->month)
		return 1;
	if (this->year == other_date->year && this->month == other_date->month && this->day > other_date->day)
		return 1;
	return 0;
}
int Date::equal(Date* other_date) {
	if (this->day == other_date->day && this->month == other_date->month && this->year == other_date->year)
		return 1;
	return 0;
}
int Date::still_in_hospital() {
	if (this->day < 1 && this->month < 1)
		return 1;
	else if (this->day > 0 && this->month > 0)
		return 0;
	cout << "MISTAKE IN EXIT DATE" << endl; // DIAGNOSTIC
	return 1;
}
void Date::print() {
	if (this->day > 0)
		cout << this->day << "-" << this->month << "-" << this->year;
	else
		cout << "(" << this->day << "-" << this->month << "-" << this->year << ")";
}
char* Date::get_char_array() {
	char* date_array = new char[15];
	if (this->still_in_hospital() == 0) {
		int indexx = 0;
		sprintf(date_array, "%d", this->day);
		strcat(date_array, "-");
		indexx = strlen(date_array);
		sprintf(date_array + indexx, "%d", this->month);
		strcat(date_array, "-");
		indexx = strlen(date_array);
		sprintf(date_array + indexx, "%d", this->year);
	}
	else
		strcpy(date_array, "--");
	return date_array;
}
Date::~Date() {
	// cout << "Date destructor" << endl;
}