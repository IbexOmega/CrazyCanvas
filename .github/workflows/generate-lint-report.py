import os, subprocess, sys, getopt
from subprocess import run

def find_cppcheck():
	drive_letters = ['C', 'D']

	for drive_letter in drive_letters:
		cppcheck_path = drive_letter + r":\Program Files\Cppcheck\cppcheck.exe"
		if os.path.isfile(cppcheck_path):
			return cppcheck_path

	print('Failed to find cppcheck.exe')
	sys.exit(1)

def lint(cppcheck_path, report_path, ignore_path):
	args = [
		cppcheck_path,
		'--enable=all',
		'--platform=win64',
		'--template={callstack}: ({severity}) {message}',
		'--inconclusive',
		'-q',
		'LambdaEngine/*',
		'Sandbox/*',
		'Client/*',
		'Server/*',
		'CrazyCanvas/*'
	]

	if ignore_path:
		args.append(f'-i{ignore_path}')

	# Use stdout as the report
	with open(report_path, 'w') as report:
		print('Linting... ', flush=True, end='')
		subprocess.run(args, stderr=report)
		print('Finished', flush=True)
		report.close()

def main(argv):
	report_path = None
	ignore_path = None
	helpStr = '''usage: generate-lint-report.py -o <path> -i <dir>\n
		-o: path in which to store lint report\n
		-i: file or directory to ignore when linting. Wildcards are allowed, eg: \'-i src/*\''''
	try:
		opts, args = getopt.getopt(argv, "hi:o:", ["help"])
	except getopt.GetoptError:
		print("Intended usage:")
		print(helpStr)
		print(f"Used flags: {str(args)}")
		sys.exit(1)
	for opt, arg in opts:
		if opt == "-h":
			print(helpStr)
			sys.exit(1)
		elif opt == "-o":
			report_path = arg
		elif opt == "-i":
			ignore_path = arg

	if not report_path:
		print(helpStr)
		sys.exit(1)

	cppcheck_path = find_cppcheck()
	lint(cppcheck_path, report_path, ignore_path)
	sys.exit(0)

if __name__ == "__main__":
	main(sys.argv[1:])
