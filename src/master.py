import subprocess
import os

if __name__ == "__main__":
	cwd = os.getcwd()
	test_suite_path = os.path.join(cwd, "test_suite")
	stderr_path = os.path.join(cwd, "stderr.txt")

	test_path_lst = []
	for f0 in os.listdir(test_suite_path):
		upper_test_path = os.path.join(test_suite_path, f0)
		# should be folder with group of tests
		if os.path.isdir(upper_test_path):
			for f1 in os.listdir(upper_test_path):
				inner_test_path = os.path.join(upper_test_path, f1)
				# should be folder of individual test
				if os.path.isdir(inner_test_path):
					test_path_lst.append(inner_test_path)

	subprocess.run(["make", "main"])
	stderr_file = open(stderr_path, "w+")
	for test_path in test_path_lst:
		result = subprocess.run(["./main", "-t", "8", "-p", test_path + "/"], stderr=subprocess.PIPE)
		if result.stderr:
			stderr_file.write("/".join(test_path.split('/')[-2:]))
			stderr_file.write("\n")
			stderr_file.write(result.stderr.decode('utf-8'))
			stderr_file.write("\n")
	stderr_file.close()
