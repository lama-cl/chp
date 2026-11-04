--TEST--
RdKafka\Conf
--FILE--
<?php

class TestBug extends RdKafka\Conf
{
    public function set($name, $value): void
    {
        parent::set($name, $value);
    }
}

$conf = new TestBug();
$conf->set('metadata.broker.list', '127.0.0.1');

echo "done" . PHP_EOL;
--EXPECT--
done
