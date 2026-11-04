#!/usr/bin/env php
<?php

const NS = 'http://pear.php.net/dtd/package-2.1';

$doc = new DOMDocument();
if (!$doc->load(__DIR__.'/../package.xml')) {
    throw new \Exception('Failed loading package.xml');
}

$notesElem = findOneElement($doc, '/ns:package/ns:notes');
echo $notesElem->textContent;

function findOneElement(DOMDocument $doc, string $path): DOMElement
{
    $xp = new DOMXPath($doc, false);
    $xp->registerNamespace('ns', NS);

    $list = $xp->evaluate($path);
    if ($list->length !== 1) {
        throw new \Exception(sprintf(
            'XPath expression %s expected to find 1 element, but found %d',
            $path,
            $list->length,
        ));
    }

    return $list->item(0);
}
