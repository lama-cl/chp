<?php

/**
 * @generate-class-entries
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

namespace RdKafka\Metadata;

class Partition
{
    /** @implementation-alias RdKafka::__construct */
    private function __construct() {}

    /** @tentative-return-type */
    public function getId(): int {}

    /** @tentative-return-type */
    public function getErr(): int {}

    /** @tentative-return-type */
    public function getLeader(): int {}

    /** @tentative-return-type */
    public function getReplicas(): \RdKafka\Metadata\Collection {}

    /** @tentative-return-type */
    public function getIsrs(): \RdKafka\Metadata\Collection {}
}
