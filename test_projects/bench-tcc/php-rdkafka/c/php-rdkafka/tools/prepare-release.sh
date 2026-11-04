#!/bin/bash

set -e

version="$1"
baseBranch=6.x

if [[ -z "$version" ]]; then
    printf "Missing version parameter" >&2
    printf "Usage: %s <version>\n" "$0" >&2
    exit 1
fi

echo "Updating package.xml"
./tools/new-package-release.php "$1"
pecl package-validate

echo "Updating PHP_RDKAFKA_VERSION"
sed -i 's/#define PHP_RDKAFKA_VERSION.*/#define PHP_RDKAFKA_VERSION "'"$1"'"/' php_rdkafka.h

echo "Printing diff"
git diff

read -p "Commit and send pull request for version $version ? [n/N]" -n 1 -r
echo
if ! [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Ok, aborting"
    exit 1
fi

git checkout -b "release/$version" "$baseBranch"
git commit package.xml php_rdkafka.h -m "$1"
gh pr create --fill --label release --base "$baseBranch"
