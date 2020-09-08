import json, os, subprocess, sys, getopt

PAGES_REPO_OWNER    = 'IbexOmega'
PAGES_REPO_NAME     = 'CrazyCanvasPages'
REPO_DIR            = 'docs'

BENCHMARK_RESULTS_PATH_RT_ON    = 'CrazyCanvas/benchmark_results_rt_on.json'
BENCHMARK_RESULTS_PATH_RT_OFF   = 'CrazyCanvas/benchmark_results_rt_off.json'

def print_help(helpString, args):
    print('Intended usage:')
    print(helpString)
    print(f'Used flags: {str(args)}')

def pull_pages_repo(repo_URL):
    print(f'Cloning pages repo into {REPO_DIR}/')
    subprocess.run(f'git clone {repo_URL} {REPO_DIR}', shell=True, check=True)

# Gets information regarding a commit in the game repository
def get_commit_info(commit_ID):
    # GITHUB_REPOSITORY: repoOwner/repoName
    gameRepoInfo = os.environ.get('GITHUB_REPOSITORY')
    if not gameRepoInfo:
        print('Missing environment variable: GITHUB_REPOSITORY')
        sys.exit(1)
    URL = f'https://api.github.com/repos/{gameRepoInfo}/commits/{commit_ID}'

    import requests
    resp = requests.get(URL)
    if resp.status_code != 200:
        print(f'Error: {URL} returned {resp.status_code}')
        sys.exit(1)
    return resp.json()

def update_commit_data(chartData, commit_ID):
    commitInfo = get_commit_info(commit_ID)
    commitMessage = commitInfo['commit']['message'].replace('\n\n', '\n')
    chartData['commits'][commit_ID] = {
        'message': commitMessage,
        'timestamp': commitInfo['commit']['author']['date']
    }

def update_average_fps_chart(chartData, rt_on_results, rt_off_results, commit_ID):
    chartData['commitIDs'].append(commit_ID)
    chartData['rtOn'].append(rt_on_results['AverageFPS'])
    chartData['rtOff'].append(rt_off_results['AverageFPS'])

def update_peak_memory_usage_chart(chartData, rt_on_results, rt_off_results, commit_ID):
    chartData['commitIDs'].append(commit_ID)
    chartData['rtOn'].append(rt_on_results['PeakMemoryUsage'])
    chartData['rtOff'].append(rt_off_results['PeakMemoryUsage'])

def update_charts(commit_ID, repo_dir):
    print(f'Updating charts in {repo_dir}/_data/')

    with open(BENCHMARK_RESULTS_PATH_RT_ON, 'r') as benchmarkFile:
        rt_on_results = json.load(benchmarkFile)
        benchmarkFile.close()

    with open(BENCHMARK_RESULTS_PATH_RT_OFF, 'r') as benchmarkFile:
        rt_off_results = json.load(benchmarkFile)
        benchmarkFile.close()

    with open(f'{repo_dir}/_data/charts.json', 'r+') as chartsFile:
        chartsData = json.load(chartsFile)
        update_commit_data(chartsData, commit_ID)
        update_average_fps_chart(chartsData['AverageFPS'], rt_on_results, rt_off_results, commit_ID)
        update_peak_memory_usage_chart(chartsData['PeakMemoryUsage'], rt_on_results, rt_off_results, commit_ID)
        chartsFile.seek(0)
        json.dump(chartsData, chartsFile, indent=4)
        chartsFile.truncate()
        chartsFile.close()

def commit_changes(repoURL):
    subprocess.run('git add _data/*', shell=True, check=True)
    subprocess.run('git commit -m \"Update charts data\"', shell=True, check=True)
    subprocess.run(f'git remote set-url origin {repoURL}', shell=True, check=True)
    subprocess.run(f'git push {repoURL}', shell=True, check=True)

def main(argv):
    help_str = '''This script is intended to be executed by a GitHub Action Runner.\n
        Its purpose is to read existing benchmark results and push them to the repository containing the benchmark charts.'''
    try:
        opts, args = getopt.getopt(argv, 'h', ['help'])
    except getopt.GetoptError:
        print_help(help_str, args)
        sys.exit(1)

    for opt, _ in opts:
        if opt in ['-h', '--help']:
            print_help(help_str, args)
            sys.exit(1)

    commit_ID   = os.environ.get('GITHUB_SHA')
    pat         = os.environ.get('PAT') # Personal Access Token for executing authenticated git commands
    if not commit_ID or not pat:
        print('Missing environment variable: {}'.format('GITHUB_SHA' if not commit_ID else 'PAT'))
        print_help(help_str, args)
        sys.exit(1)

    repo_URL = f'https://{pat}:x-oauth-basic@github.com/{PAGES_REPO_OWNER}/{PAGES_REPO_NAME}.git'
    pull_pages_repo(repo_URL)
    update_charts(commit_ID[:7], REPO_DIR)
    os.chdir(REPO_DIR)
    commit_changes(repo_URL)

if __name__ == '__main__':
    main(sys.argv[1:])
