--TEST--
RdKafka\Conf::setDefaultTopicConf()
--FILE--
<?php

$conf = new RdKafka\Conf();

echo "Setting valid topic conf\n";
$conf->setDefaultTopicConf(new RdKafka\TopicConf());

echo "Setting invalid topic conf\n";
try {
    $conf->setDefaultTopicConf($conf);
} catch (Error $e) {
    printf("%s: %s\n", $e::class, $e->getMessage());
}

--EXPECTF--
Setting valid topic conf

Deprecated: Method RdKafka\Conf::setDefaultTopicConf() is deprecated in %s%econf_setDefaultTopicConf.php on line 6
Setting invalid topic conf

Deprecated: Method RdKafka\Conf::setDefaultTopicConf() is deprecated in %s%econf_setDefaultTopicConf.php on line 10
TypeError: RdKafka\Conf::setDefaultTopicConf(): Argument #1 ($topic_conf) must be of type RdKafka\TopicConf, RdKafka\Conf given
