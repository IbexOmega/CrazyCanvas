import json, os, subprocess, sys, getopt

BENCHMARK_RESULTS_PATH          = 'benchmark_results.json'
BENCHMARK_RESULTS_PATH_RT_ON    = 'benchmark_results_rt_on.json'
BENCHMARK_RESULTS_PATH_RT_OFF   = 'benchmark_results_rt_off.json'

ENGINE_CONFIG_PATH      = '../engine_config.json'
TEMP_ENGINE_CONFIG_PATH = '../engine_config_copy.json'

ENGINE_CONFIG = {
	'WindowSize': [1920, 1080],
	'FixedTimestep': 60,
	'RayTracingEnabled': True,
	'ShowRenderGraph': False,
	'ShowDemo': False,
	'Debugging': False,
	'CameraFOV': 90.0,
	'CameraNearPlane': 0.001,
	'CameraFarPlane': 1000.0
}

def print_help(help_string, args):
	print('Intended usage:')
	print(help_string)
	print(f'Used flags: {str(args)}')

def remove_existing_benchmark_files():
	for file_name in [BENCHMARK_RESULTS_PATH, BENCHMARK_RESULTS_PATH_RT_ON, BENCHMARK_RESULTS_PATH_RT_OFF]:
		if os.path.exists(file_name):
			os.remove(file_name)

def set_engine_config(ray_tracing_enabled):
	with open(ENGINE_CONFIG_PATH, 'w+', newline='\n') as cfgFile:
		config = ENGINE_CONFIG
		config['RayTracingEnabled'] = ray_tracing_enabled
		cfgFile.seek(0)
		json.dump(config, cfgFile, indent=4)
		cfgFile.truncate()
		cfgFile.close()

def run_benchmark(bin_path, ray_tracing_enabled):
	set_engine_config(ray_tracing_enabled)
	print('Benchmarking with ray tracing {}... '.format('enabled' if ray_tracing_enabled else 'disabled'), end='', flush=True)
	completed_process = subprocess.run([bin_path], capture_output=True)
	if completed_process.returncode != 0:
		print(f'Failed:\n{str(completed_process.stdout)}\n\n{str(completed_process.stderr)}')
		sys.exit(1)

	print(' Success')

def main(argv):
	help_str = '''usage: --bin <binpath>\n
		bin: path to application binary'''
	try:
		opts, args = getopt.getopt(argv, 'h', ['help', 'bin='])
	except getopt.GetoptError:
		print_help(help_str, args)
		sys.exit(1)

	bin_path = None
	for opt, arg in opts:
		if opt in ['-h', '--help']:
			print_help(help_str, args)
			sys.exit(1)
		if opt == '--bin':
			bin_path = arg

	if not bin_path:
		print('Missing argument')
		print_help(help_str, args)
		sys.exit(1)

	os.chdir('CrazyCanvas')
	bin_path = '../' + bin_path

	remove_existing_benchmark_files()

	# Temporarily rename engine config file
	if not os.path.exists(TEMP_ENGINE_CONFIG_PATH):
		os.rename(ENGINE_CONFIG_PATH, TEMP_ENGINE_CONFIG_PATH)

	run_benchmark(bin_path, ray_tracing_enabled=True)
	os.rename(BENCHMARK_RESULTS_PATH, BENCHMARK_RESULTS_PATH_RT_ON)

	run_benchmark(bin_path, ray_tracing_enabled=False)
	os.rename(BENCHMARK_RESULTS_PATH, BENCHMARK_RESULTS_PATH_RT_OFF)

	# Restore original engine config
	os.remove(ENGINE_CONFIG_PATH)
	os.rename(TEMP_ENGINE_CONFIG_PATH, ENGINE_CONFIG_PATH)

if __name__ == '__main__':
	main(sys.argv[1:])
