#!/bin/sh

set -e

cd php-rdkafka

echo "Checking version consistency"

CODE_VERSION="$(grep PHP_RDKAFKA_VERSION php_rdkafka.h|cut -d'"' -f2)"
PACKAGE_VERSION="$(grep -m 1 '<release>' package.xml|cut -d'>' -f2|cut -d'<' -f1)"

if ! [ "$CODE_VERSION" = "$PACKAGE_VERSION" ]; then
    printf "Version in php_rdkafka.h does not match version in package.xml: '%s' vs '%s'" "$CODE_VERSION" "$PACKAGE_VERSION" >&2
    exit 1
fi

echo "Packaging"

pecl package

echo "Installing package.xml"

mv "./rdkafka-$PACKAGE_VERSION.tgz" rdkafka.tgz
sudo pecl install ./rdkafka.tgz

echo "Checking that all test files was included"

sudo pecl list-files rdkafka|grep ^test|sed 's@.*/tests/@@'|sort > installed-test-files
find tests/ -type f|sed 's@^tests/@@'|sort > repository-test-files

if ! diff -u repository-test-files installed-test-files; then
    echo "Some test files are missing from package.xml (see diff above)" >&2
    exit 1
fi
