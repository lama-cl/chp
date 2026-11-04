#!/bin/sh

set -e

echo "Building php-rdkafka with PHP version:"
php --version

cd php-rdkafka

PACKAGE_VERSION="$(grep -m 1 '<release>' package.xml|cut -d'>' -f2|cut -d'<' -f1)"

pecl package

if [ $MEMORY_CHECK -eq 1 ]; then
    PHP_RDKAFKA_CFLAGS="-Wall -Werror -Wno-deprecated-declarations"
fi

sudo CFLAGS="$PHP_RDKAFKA_CFLAGS" pecl install "./rdkafka-$PACKAGE_VERSION.tgz"

echo "extension=rdkafka.so"|sudo tee /usr/local/etc/php/rdkafka.ini >/dev/null
