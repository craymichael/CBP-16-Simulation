#!/usr/bin/env bash

# Get the directory this script exists in.
# https://stackoverflow.com/a/246128/6557588
SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do # resolve $SOURCE until the file is no longer a symlink
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE" # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

echo Script running from "\"$DIR\""

# Pull each file if it is needed in parallel
cd "${DIR}/../data/"

echo "If the URL http://hpca23.cse.tamu.edu/cbp2016/ does not resolve for you, follow the instructions for data download in the README instead!"
URLS=('http://hpca23.cse.tamu.edu/cbp2016/cbp2016.final.tar.gz'
      'http://hpca23.cse.tamu.edu/cbp2016/trainingTraces.Final.tar'
      'http://hpca23.cse.tamu.edu/cbp2016/evaluationTraces.Final.tar'
      'http://hpca23.cse.tamu.edu/cbp2016/cbp2016_evaluation_results.tar.gz'
      'http://hpca23.cse.tamu.edu/cbp2016/MD5SUM.txt')
echo ${URLS[@]} | xargs -n 1 -P 8 wget -nc

# MD5SUM check
if [[ ! -f ".md5sum_verified" ]];
then
  echo "Checking MD5SUMs - this may take a while"
  if ! md5sum -c MD5SUM.txt;
  then
    echo -e "\e[31m\e[1mAborting\e[22m\e[21m: MD5SUMs do not match up\e[0m"
    exit 1
  else
    # Use file to cache md5sum result - larger files take a long time to verify
    echo "$(date)" > .md5sum_verified
  fi
else
  echo "MD5SUMs already verified, skipping now to save time"
fi

echo -e "\nInstalling requisite programs - attempting to detect your linux distribution..."
if command -v yum > /dev/null;
then
  echo "Detected yum as the package manager, installing..."
  yum -y install gcc-c++
  yum -y install boost
  yum -y install boost-devel
elif command -v apt-get > /dev/null;
then
  echo "Detected apt as the package manager, installing..."
  sudo apt-get update -y
  sudo apt-get install -y libboost-all-dev
  sudo apt-get install -y g++
elif command -v pacman > /dev/null;
then
  echo "Detected pacman as the package manager, installing..."
  sudo pacman -S boost-libs --noconfirm --needed
  sudo pacman -S boost --noconfirm --needed
  sudo pacman -S gcc --noconfirm --needed
else
  echo -e "\e[33m\e[1mWarning\e[22m\e[21m: Unable to detect your package manager, please ensure that you have the"\
           "boost library, boost development headers, and gcc/g++ installed, otherwise building CBP will fail.\e[0m"
fi

echo -e "\nExtracting and Building the Simulator..."
tar --skip-old-files -xzf cbp2016.final.tar.gz -C ..
echo -e "Done\n"
cd ../cbp2016.eval/sim
# make the sim
make clean
if ! make;
then
  echo -e "\e[31m\e[1mAborting\e[22m\e[21m: Failed to make the simulator.\e[0m"
  exit 1
fi
echo -e "\n\e[32mSuccessfully built the CBP-16 simulator.\e[0m"

# extract the traces
echo "Extracting the training traces to traces/..."
cd ..
tar --skip-old-files -xf ../data/trainingTraces.Final.tar -C .
echo "Done. Extracting the evaluation traces to evaluationTraces/..."
tar --skip-old-files -xf ../data/evaluationTraces.Final.tar -C .
echo "Done. Extracting the evaluation results to results/..."
cd results/
tar --skip-old-files -xf ../../data/cbp2016_evaluation_results.tar.gz -C .
echo -e "\n\e[32mSuccessfully downloaded CBP-16 files and set up project.\e[0m"
