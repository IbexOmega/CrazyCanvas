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
def printWarning(line, filesToLint):
    fileRegex       = r"\[([^:]+):([0-9]+)\]"
    anything        = ".*"
    categoryRegex   = r"(\([^)]+\))"
    messageRegex    = "(.*?)$"
    match           = re.search(rf"{fileRegex}(?:{anything}{fileRegex})?{anything}{categoryRegex}\s{messageRegex}", line)
    if match is None or len(match.groups()) != 6:
        print(f"Failed to regex search string: {line}")
        return False

    searchResults   = match.groups()

    firstFileName   = searchResults[0]
    firstLineNr     = searchResults[1]
    lintCategory    = searchResults[4]
    message         = searchResults[5]
    secondFileName  = ""
    secondLineNr    = ""

    outMessage = ""
    if searchResults[2] is not None and searchResults[3] is not None:
        # The line contains two files. Have the second one be printed in the output message
        secondFileName  = searchResults[2]
        secondLineNr    = searchResults[3]
        outMessage += f"[{secondFileName}:{secondLineNr}]: "

    if firstFileName not in filesToLint and secondFileName not in filesToLint:
        return False

    outMessage += f"{lintCategory} {message}"
    print(f"::warning file={firstFileName},line={firstLineNr}::{outMessage}")
    return True

def readReport(fileName, filesToLint):
    suppressedMessages = []
    warningCount = 0

    with open(fileName) as file:
        for line in file:
            if isSuppressed(line):
                suppressedMessages.append(line)
            elif printWarning(line, filesToLint):
                warningCount += 1

    print(f"Warnings: {warningCount}, suppressed messages: {len(suppressedMessages)}")
    print("Suppressed messages:")
    for suppressedMessage in suppressedMessages:
        print(suppressedMessage, end='')

def getFilesToLint(modifiedFilesPath, addedFilesPath):
    modifiedFiles = json.load(open(modifiedFilesPath, 'r')) if modifiedFilesPath != "" else []
    addedFiles = json.load(open(addedFilesPath, 'r')) if addedFilesPath != "" else []
    print(modifiedFiles)
    print(addedFiles)
    return modifiedFiles + addedFiles

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

    filesToLint = getFilesToLint(modifiedFilesPath, addedFilesPath)
    readReport(inputFile, filesToLint)

if __name__ == "__main__":
    main(sys.argv[1:])
