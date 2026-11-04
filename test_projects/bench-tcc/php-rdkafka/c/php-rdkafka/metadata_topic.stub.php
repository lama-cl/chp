<?php

/**
 * @generate-class-entries
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

namespace RdKafka\Metadata;

class Topic
{
    /** @implementation-alias RdKafka::__construct */
    private function __construct() {}

    /** @tentative-return-type */
    public function getTopic(): string {}

    /** @tentative-return-type */
    public function getErr(): int {}

    /** @tentative-return-type */
    public function getPartitions(): \RdKafka\Metadata\Collection {}
}
