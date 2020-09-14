import json, os, re, sys, getopt

# Regexes that will catch messages to be suppressed
SUPPRESSED_REGEXES = [
    "Class '.*' has a constructor with 1 argument that is not explicit",
    "Consider using std::transform",
    "The function '.*' is never used",
    "Class '.*' does not have a .* which is recommended since it has dynamic memory",
    r"^: \(information\)"
]

def isSuppressed(line):
    for suppressedRegex in SUPPRESSED_REGEXES:
        if re.search(suppressedRegex, line) is not None:
            return True
    return False

# Returns true if a warning was printed
def print_warning(line, filesToLint):
    file_regex      = r"\[([^:]+):([0-9]+)\]"
    anything        = ".*"
    category_regex  = r"(\([^)]+\))"
    message_regex   = "(.*?)$"
    match           = re.search(rf"{file_regex}(?:{anything}{file_regex})?{anything}{category_regex}\s{message_regex}", line)
    if match is None or len(match.groups()) != 6:
        print(f"Failed to regex search string: {line}")
        return False

    search_results   = match.groups()

    first_file_name     = search_results[0].replace('\\', '/')
    first_line_nr       = search_results[1]
    second_file_name    = search_results[2]
    second_line_nr      = search_results[3]
    lint_category       = search_results[4]
    message             = search_results[5]

    out_message = ''
    if second_file_name and second_line_nr:
        # The line contains two files. Have the second one be printed in the output message
        second_file_name = second_file_name.replace('\\', '/')
        out_message += f'[{second_file_name}:{second_line_nr}]: '

    if first_file_name not in filesToLint and second_file_name not in filesToLint:
        return False

    out_message += f"{lint_category} {message}"
    print(f'::warning file={first_file_name},line={first_line_nr}::{out_message}')
    return True

def read_report(fileName, filesToLint):
    suppressedMessages = []
    warningCount = 0

    with open(fileName) as file:
        for line in file:
            if isSuppressed(line):
                suppressedMessages.append(line)
            elif print_warning(line, filesToLint):
                warningCount += 1

    print(f'Warnings: {warningCount}, suppressed messages: {len(suppressedMessages)}')
    print('Suppressed messages:')
    for suppressedMessage in suppressedMessages:
        print(suppressedMessage, end='')

def get_files_to_lint(modifiedFilesPath, addedFilesPath):
    modified_files = json.load(open(modifiedFilesPath, 'r')) if modifiedFilesPath != "" else []
    added_files = json.load(open(addedFilesPath, 'r')) if addedFilesPath != "" else []

    print(f'Modified files: {modified_files}\nAdded Files: {added_files}')
    return modified_files + added_files

def main(argv):
    inputFile = ""
    modifiedFilesPath = ""
    addedFilesPath = ""
    helpStr = '''usage: read-lint-report.py --report <path> --modified-files <path> --added-files <path>\n
        modified-files: path to csv file containing file paths of files modified in a merge request\n
        added-files: path to csv file containing file paths of files added in a merge request'''
    try:
        opts, args = getopt.getopt(argv, "h", ["report=", "modified-files=", "added-files="])
    except getopt.GetoptError:
        print("Intended usage:")
        print(helpStr)
        print(f"Used flags: {str(args)}")
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-h":
            print(helpStr)
            sys.exit(1)
        elif opt == "--report":
            inputFile = arg
        elif opt == "--modified-files":
            modifiedFilesPath = arg
        elif opt == "--added-files":
            addedFilesPath = arg

    filesToLint = get_files_to_lint(modifiedFilesPath, addedFilesPath)
    read_report(inputFile, filesToLint)

if __name__ == "__main__":
    main(sys.argv[1:])
