<?php

/**
 * @generate-class-entries
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

namespace RdKafka\Metadata;

class Broker
{
    /** @implementation-alias RdKafka::__construct */
    private function __construct() {}

    /** @tentative-return-type */
    public function getId(): int {}

    /** @tentative-return-type */
    public function getHost(): string {}

    /** @tentative-return-type */
    public function getPort(): int {}
}
