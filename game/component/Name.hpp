#pragma once

#include <string>

class Name {
public:
	Name(const std::string& name) : name(name) { }

	std::string name; // replace with char name[] with set and to_string functions
};