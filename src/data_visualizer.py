import matplotlib.pyplot as plt

# test_dir_path = "/afs/andrew.cmu.edu/usr3/aadulla/private/15418/final_project/test_0/"
test_dir_path = "C:/Users/Ashwin Adulla/Desktop/2019-2020/Spring/15-418/15418/final_project/test_0/"
setup_clocks = 500
filtered_len = 100

def get_data_paths(test_dir_path):
	tx_stats_path = test_dir_path + "tx_stats.txt"
	rx_stats_path = test_dir_path + "rx_stats.txt"
	failed_stats_path = test_dir_path + "failed_stats.txt"
	buffer_stats_path = test_dir_path + "buffer_stats.txt"
	return tx_stats_path, rx_stats_path, failed_stats_path, buffer_stats_path

def extract_data(data_file_path, data_type):
	data_file = open(data_file_path)
	data_lst = []

	for str_val in data_file.readlines():
		str_val = str_val.strip()
		if (data_type == "int"):
			data_lst.append(int(str_val))
		elif (data_type == "float"):
			data_lst.append(float(str_val))

	data_file.close()
	return data_lst

def filter_data(data_lst):
	padding = len(data_lst)%filtered_len
	for i in range(padding):
		data_lst.append(0)

	window_size = int(len(data_lst)/filtered_len)
	filtered_data_lst = []
	for i in range(filtered_len):
		avg = sum(data_lst[i*window_size:(i+1)*window_size])/window_size
		filtered_data_lst.append(avg)
	return filtered_data_lst

if __name__ == '__main__':
	tx_stats_path, rx_stats_path, failed_stats_path, buffer_stats_path= get_data_paths(test_dir_path)

	tx_stats_lst = extract_data(tx_stats_path, "int") 
	rx_stats_lst = extract_data(rx_stats_path, "int") 
	failed_stats_lst = extract_data(failed_stats_path, "int") 
	buffer_stats_lst = extract_data(buffer_stats_path, "float") 
	clock_lst = [i for i in range(len(tx_stats_lst))]

	# setup_clocks = 5000
	# tx_stats_lst = tx_stats_lst[setup_clocks:]
	# rx_stats_lst = rx_stats_lst[setup_clocks:]
	# failed_stats_lst = failed_stats_lst[setup_clocks:]
	# buffer_stats_lst = buffer_stats_lst[setup_clocks:]
	# clock_lst = clock_lst[setup_clocks:]

	tx_stats_lst = filter_data(tx_stats_lst)
	rx_stats_lst = filter_data(rx_stats_lst)
	failed_stats_lst = filter_data(failed_stats_lst)
	buffer_stats_lst = filter_data(buffer_stats_lst)
	clock_lst = filter_data(clock_lst)

	fig, axs = plt.subplots(2, 2)

	axs[0, 0].plot(clock_lst, tx_stats_lst)
	axs[0, 0].set(xlabel="Clock Cyle Count", ylabel="Number of Messages Transmitted")

	axs[1, 0].plot(clock_lst, rx_stats_lst)
	axs[1, 0].set(xlabel="Clock Cyle Count", ylabel="Number of Messages Received")


	axs[0, 1].plot(clock_lst, failed_stats_lst)
	axs[0, 1].set(xlabel="Clock Cyle Count", ylabel="Number of Contentions")


	axs[1, 1].plot(clock_lst, buffer_stats_lst)
	axs[1, 1].set(xlabel="Clock Cyle Count", ylabel="Buffer Efficiency")

	plt.show()

