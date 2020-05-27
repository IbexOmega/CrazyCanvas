DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

git submodule update
git submodule foreach --recursive git pull origin master