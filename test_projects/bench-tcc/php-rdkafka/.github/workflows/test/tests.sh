#!/bin/sh

set -xve

cd php-rdkafka

if [ ${MEMORY_CHECK:-0} -eq 1 ]; then
      echo "Enabling memory checking"
      showmem=--show-mem
      checkmem=-m
fi

cp tests/test_env.php.sample tests/test_env.php

PHP=$(which php)
sudo REPORT_EXIT_STATUS=1 TEST_PHP_EXECUTABLE="$PHP" "$PHP" /usr/local/lib/php/build/run-tests.php -q $checkmem --show-diff $showmem
