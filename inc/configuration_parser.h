#include <map>

class Config_Parser {
private:
	std::map<std::string, void*> parameter_key_to_val_map;
public:
	Config_Parser();
	void initialize_parameter_key(std::string parameter_key);
	void* get_parameter_value(std::string parameter_key)
	void parse_config_file(char* config_file)
}