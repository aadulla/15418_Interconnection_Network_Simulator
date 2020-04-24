#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <cassert>
using namespace std;

#include "config_parser.h"

Config_Parser::Config_Parser () {
	this->parameter_key_to_val_map = new map<string, string>;
};

void Config_Parser::initialize_parameter_key (string parameter_key) {
	this->parameter_key_to_val_map->insert({parameter_key, ""});
}

string Config_Parser::get_string_parameter_value (string parameter_key) {
	string parameter_val = this->parameter_key_to_val_map->find(parameter_key)->second;
	return parameter_val;
}

uint32_t Config_Parser::get_int_parameter_value (string parameter_key) {
	string parameter_val = this->parameter_key_to_val_map->find(parameter_key)->second;
	uint32_t int_parameter_val = stoi(parameter_val);
	return int_parameter_val;
}

void Config_Parser::parse_config_file (string config_file_path) {
	// open file
	ifstream config_file(config_file_path);

	// read file line by line
	string config_line;
	while (getline(config_file, config_line)) {
		size_t colon_idx = config_line.find_first_of(":");
		if (colon_idx == string::npos) continue;

		string parameter_key = config_line.substr(0, colon_idx);
		string parameter_value = config_line.substr(colon_idx + 2);
		parameter_value.pop_back(); // remove trailing newline character
		this->parameter_key_to_val_map->find(parameter_key)->second = parameter_value;
	}

	// close file
	config_file.close();
}

void Config_Parser::print () {
	for (auto itr=this->parameter_key_to_val_map->begin(); itr != this->parameter_key_to_val_map->end(); itr++) {
		cout << itr->first << " --> " << itr->second << endl;
	}
}

