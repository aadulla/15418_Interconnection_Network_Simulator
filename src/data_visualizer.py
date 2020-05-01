import copy
import matplotlib.pyplot as plt
import pandas as pd
import sys
import os

def get_data_paths(test_dir_path):
	tx_stats_path = os.path.join(test_dir_path, "tx_stats.txt")
	rx_stats_path = os.path.join(test_dir_path, "rx_stats.txt")
	stalls_stats_path = os.path.join(test_dir_path, "stalls_stats.txt")
	buffers_stats_path = os.path.join(test_dir_path, "buffers_stats.txt")
	transmissions_stats_path = os.path.join(test_dir_path, "transmissions_stats.txt")
	return tx_stats_path, rx_stats_path, stalls_stats_path, buffers_stats_path, transmissions_stats_path

def extract_transmissions_data(data_file_path):
	data_file = open(data_file_path)
	data_lines = data_file.readlines()

	headers = data_lines[0].split()
	data_dict = dict()
	for i, header in enumerate(headers):
		headers[i] = header.lower()
		data_dict.update({headers[i]:[]})

	for data_line in data_lines[1:]:
		for i, data in enumerate(data_line.split()):
			data = int(data)
			data_dict[headers[i]].append(data)

	return data_dict

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

def plot_time_series(ax, stats_df, rolling_window, x_label, y_label, title, legend):
	stats_df.rolling(rolling_window).mean().plot(ax=ax)
	ax.set(xlabel=x_label, ylabel=y_label, title=title)
	ax.legend(legend)
	# fig = ax.get_figure()
	# fig.set_size_inches(x_size, y_size)
	# plt.show()

def plot_kde(ax, stats_df, x_label, y_label, title, legend, x_low, x_high, bw):
	ax = stats_df.plot(kind='kde', bw_method=bw, ax=ax)
	ax.set(xlabel=x_label, ylabel=y_label, title=title)
	ax.legend(legend)
	ax.set_xbound(lower=x_low, upper=x_high)
	# fig = ax.get_figure()
	# fig.set_size_inches(x_size, y_size)
	# plt.show()

def pad(data_dict):
	max_len = 0
	for key, data_stats in data_dict.items():
		if max_len < len(data_stats): max_len = len(data_stats)
	for key, data_stats in data_dict.items():
		for i in range(len(data_stats), max_len, 1):
			data_stats.append(0)

def time_series_subplot(ax, dict_lst, key, x_label, y_label, titles, legend, rolling_window):
	ax_rows = len(ax)
	ax_cols = len(ax[0])
	
	i = 0
	for r in range(ax_rows):
		for c in range(ax_cols):
			plot_time_series(ax[r,c],
							 dict_lst[i][key], 
							 rolling_window=rolling_window,
							 x_label=x_label, 
							 y_label=y_label, 
							 title=titles[i],
							 legend=legend
							 )
			i += 1

def kde_subplot(ax, dict_lst, key, x_label, y_label, titles, legend,  x_low, x_high, bw):
	ax_rows = len(ax)
	ax_cols = len(ax[0])
	
	i = 0
	for r in range(ax_rows):
		for c in range(ax_cols):
			plot_kde(ax[r,c],
					 dict_lst[i][key],
					 x_label=x_label, 
					 y_label=y_label, 
					 title=titles[i],
					 legend=legend,
					 x_low=x_low,
					 x_high=x_high,
					 bw=bw
					 )
			i += 1

def data_parser(test_dir_path_lst):
	tx_stats_dict = dict()
	rx_stats_dict = dict()
	stalls_stats_dict = dict()
	buffers_stats_dict = dict()
	latency_stats_dict = dict()
	size_stats_dict = dict()
	distance_stats_dict = dict()

	for test_dir_path in test_dir_path_lst:
		tx_stats_path, rx_stats_path, stalls_stats_path, buffers_stats_path, transmissions_stats_path = get_data_paths(test_dir_path)
		tx_stats = extract_data(tx_stats_path, "int") 
		rx_stats = extract_data(rx_stats_path, "int") 
		stalls_stats = extract_data(stalls_stats_path, "int")
		buffers_stats = extract_data(buffers_stats_path, "float")
		transmissions_data_dict = extract_transmissions_data(transmissions_stats_path)

		tx_stats_dict.update({test_dir_path: tx_stats})
		rx_stats_dict.update({test_dir_path: rx_stats})
		stalls_stats_dict.update({test_dir_path: stalls_stats})
		buffers_stats_dict.update({test_dir_path: buffers_stats})
		latency_stats_dict.update({test_dir_path: transmissions_data_dict['latency']})
		size_stats_dict.update({test_dir_path: transmissions_data_dict['size']})
		distance_stats_dict.update({test_dir_path: transmissions_data_dict['distance']})

	pad(tx_stats_dict)
	pad(rx_stats_dict)
	pad(stalls_stats_dict)
	pad(buffers_stats_dict)

	tx_stats_df = pd.DataFrame.from_dict(tx_stats_dict) 
	rx_stats_df = pd.DataFrame.from_dict(rx_stats_dict) 
	stalls_stats_df = pd.DataFrame.from_dict(stalls_stats_dict) 
	buffers_stats_df = pd.DataFrame.from_dict(buffers_stats_dict)

	latency_stats_df = pd.DataFrame.from_dict(latency_stats_dict) 
	size_stats_df = pd.DataFrame.from_dict(size_stats_dict) 
	distance_stats_df = pd.DataFrame.from_dict(distance_stats_dict) 

	global_stats_df_dict = {"tx": tx_stats_df,
							"rx": rx_stats_df,
							"stalls": stalls_stats_df,
							"buffers": buffers_stats_df,
							"latency": latency_stats_df,
							"size": size_stats_df,
							"distance": distance_stats_df}

	return global_stats_df_dict

