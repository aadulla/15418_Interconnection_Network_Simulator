import os
import sys
import copy
import itertools
import shutil

yes_permute = 1
no_permute = 0

base_config_dict = {"Network Type:": "Mesh",
					"Number of Processors:": 100,
					"Number of Routers:": 100,
					"Router Buffer Capacity:": 13,
					"Number of Virtual Channels:": 5,
					"Packet Width:": 5,
					"Number of Data Flits Per Packet:": 10,
					"Routing Algorithm:": "Mesh XY",
					"Flow Control Algorithm:": "Cut Through",
					"Flow Control Granularity:": "Packet",
					"Number of Messages:": 1000,
					"Lower Message Size:": 20,
					"Upper Message Size:": 50,
					"Message Size Distribution:": "Random",
					"Message Node Distribution:": "Uniform",}

global_test_suite_dict = {
				"routing_+_flow_control_+_message_size_+_message_distribution": [
																					[
																						[["Routing Algorithm:"		 , ["Mesh XY", "Mesh Adaptive"]],
																						 ["Flow Control Algorithm:"	 , ["Store Forward", "Cut Through"]],
																						 ["Flow Control Granularity:", ["Packet", "Flit"]]], 
																						yes_permute],
																					 [
																						[["Lower Message Size:", [10,   50,   100, 500]],
																						 ["Upper Message Size:", [10,   50,   100, 500]],
																						 ["Number of Messages:", [5000, 1000, 500, 100]]],
																						no_permute],
																					 [
																						[["Message Size Distribution:", ["Uniform"]]],
																						no_permute]
																				],
											
				"routing_+_buffer_capacity_+_virtual_channel_+_granularity": [
																					[
																						[["Router Buffer Capacity:"	   ,  [ 3,  4,   6,  10, 15, 20]],
																						 ["Number of Virtual Channels:" , [20, 15,  10,   6,  4,  3]]],
																						no_permute],
																					[
																						[["Routing Algorithm:", ["Mesh XY", "Mesh Adaptive"]],
																						 ["Flow Control Granularity:", ["Packet","Flit"]]],
																						yes_permute]
																				]
				}

def flatten(lst):
	if lst == []:
		return lst
	if isinstance(lst[0], list):
		return flatten(lst[0]) + flatten(lst[1:])
	return lst[:1] + flatten(lst[1:])

def convert_tups_to_lsts(lst_of_tups):
	lst_of_lsts = []
	for tup in lst_of_tups:
		lst_of_lsts.append(list(tup))
	return lst_of_lsts

def create_opt_group_dict_lst(opt_group_lst):
	opt_group_dict_lst = []
	for opt in opt_group_lst:
		opt_dict_lst = []
		opt_name = opt[0]
		opt_vals = opt[1]
		for opt_val in opt_vals:
			opt_dict_lst.append({opt_name: opt_val})
		opt_group_dict_lst.append(opt_dict_lst)

	return opt_group_dict_lst	


def yes_permute_options(opt_group_lst):
	opt_group_dict_lst = create_opt_group_dict_lst(opt_group_lst)
	product_tups = list(itertools.product(*opt_group_dict_lst))
	return convert_tups_to_lsts(product_tups)

def no_permute_options(opt_group_lst):
	opt_group_dict_lst = create_opt_group_dict_lst(opt_group_lst)
	zipped_tups = list(zip(*opt_group_dict_lst))
	return convert_tups_to_lsts(zipped_tups)

def create_full_config_dicts(options):
	opt_group_dict_lsts = []
	for opt_group in options:
		should_permute = opt_group[1]
		if should_permute == yes_permute: 
			opt_group_dict_lsts.append(yes_permute_options(opt_group[0]))
		else:
			opt_group_dict_lsts.append(no_permute_options(opt_group[0]))

	opt_config_dict_tups = list(itertools.product(*opt_group_dict_lsts))
	opt_config_dict_lsts = convert_tups_to_lsts(opt_config_dict_tups)

	for i, opt_config_dict_lst in enumerate(opt_config_dict_lsts):
		opt_config_dict_lsts[i] = flatten(opt_config_dict_lst)

	full_config_dict_lst = []
	for opt_config_dict_lst in opt_config_dict_lsts:
		full_config_dict = copy.deepcopy(base_config_dict)

		for opt_config_dict in opt_config_dict_lst:
			full_config_dict.update(opt_config_dict)

		full_config_dict_lst.append(full_config_dict)

	return full_config_dict_lst, opt_config_dict_lsts

def create_config_files(full_config_dict_lst, opt_config_dict_lsts, parent_test_dir_path):
	# create subfolders with config.txt files for ecah individual test
	for i, full_config_dict	in enumerate(full_config_dict_lst):
		child_test_dir_path =  os.path.join(parent_test_dir_path, "test_" + str(i))
		os.mkdir(child_test_dir_path)

		config_file_path = os.path.join(child_test_dir_path, "config.txt")
		config_file = open(config_file_path,"w+")
		for key, val in full_config_dict.items():
			config_file.write(key + " " + str(val) + "\n")
		config_file.close()

	# create file in parent dir to map child folder names to test configs
	parent_tests_contents_path = os.path.join(parent_test_dir_path, "test_contents.txt")
	parent_tests_contents_file = open(parent_tests_contents_path,"w+")

	for i, opt_config_dict_lst in enumerate(opt_config_dict_lsts):
		parent_tests_contents_file.write("TEST " + str(i) + "\n")
		for opt_config_dict	in opt_config_dict_lst:
			for key, val in opt_config_dict.items():
					parent_tests_contents_file.write(key + " " + str(val) + "\n")
		parent_tests_contents_file.write("#"*50)
		parent_tests_contents_file.write("\n")

	parent_tests_contents_file.close()

def delete_dir(dir_path):
	for f in os.listdir(dir_path):
		new_path = os.path.join(dir_path, f)
		if os.path.isfile(new_path):
			os.remove(new_path)
		elif os.path.isdir(new_path):
			delete_dir(new_path)
	os.rmdir(dir_path)

def create_test_suite(test_suite_dict):
	cwd = os.getcwd()
	test_suite_path = os.path.join(cwd, "test_suite")

	if os.path.exists(test_suite_path):
		delete_dir(test_suite_path)
	os.mkdir(test_suite_path)

	for test_dir_name, options in test_suite_dict.items():
		test_dir_path = os.path.join(test_suite_path, test_dir_name)
		os.mkdir(test_dir_path)

		full_config_dict_lst, opt_config_dict_lsts = create_full_config_dicts(options)
		create_config_files(full_config_dict_lst, opt_config_dict_lsts, test_dir_path)

if __name__ == "__main__":
	create_test_suite(global_test_suite_dict)


