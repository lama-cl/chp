<?php

/**
 * @generate-class-entries
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

namespace RdKafka;

class KafkaErrorException extends Exception
{
    private string $error_string;

    private bool $isFatal;

    private bool $isRetriable;

    private bool $transactionRequiresAbort;

    public function __construct(string $message, int $code, string $error_string, bool $isFatal, bool $isRetriable, bool $transactionRequiresAbort) {}

    /** @tentative-return-type */
    public function getErrorString(): string {}

    /** @tentative-return-type */
    public function isFatal(): bool {}

    /** @tentative-return-type */
    public function isRetriable(): bool {}

    /** @tentative-return-type */
    public function transactionRequiresAbort(): bool {}
}
