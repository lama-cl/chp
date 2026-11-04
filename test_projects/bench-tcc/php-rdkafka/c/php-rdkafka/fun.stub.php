<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

function rd_kafka_get_err_descs(): array {}

function rd_kafka_err2name(int $err): ?string {}

function rd_kafka_err2str(int $err): ?string {}

/** @deprecated */
function rd_kafka_errno2err(int $errnox): int {}

/** @deprecated */
function rd_kafka_errno(): int {}

function rd_kafka_offset_tail(int $cnt): int {}

function rd_kafka_thread_cnt(): int {}
