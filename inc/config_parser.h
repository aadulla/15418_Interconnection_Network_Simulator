#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <map>
#include <string>
using namespace std;

class Config_Parser {

private:
	map<string, string>* parameter_key_to_val_map;

public:
	Config_Parser();
	void initialize_parameter_key(string parameter_key);
	string get_string_parameter_value(string parameter_key);
	uint32_t get_int_parameter_value(string parameter_key);
	void parse_config_file(string config_file_path);
	void print();
};

#endif /* CONFIG_PARSER_H */