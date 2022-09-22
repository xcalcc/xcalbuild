#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# The tarball of the windows pp results
TARBALL=$1

mkdir windows-results
cd windows-results
tar xvf ../$1
mkdir logs

for $project in `ls $DIR/windows-results/win-projects`; do
    $DIR/analyse-preprocessed.sh ${DIR}/windows-results/win-projects/$project
    cp -r .scan_log logs/${project}-scan-log
    cp -r scan_result logs/${project}-scan-result || echo "No analysis result"
done

cp win-projects/*.log logs/