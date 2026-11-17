Headers that thttpd's Makefile generates at build time (the Makefile is not
part of the benchmark tree). Produced from `mime_encodings.txt` and
`mime_types.txt` with thttpd 2.29's own Makefile rule:

    sed < X.txt > X.h -e 's/#.*//' -e 's/[ \t]*$//' -e '/^$/d' \
        -e 's/[ \t][ \t]*/", 0, "/' -e 's/^/{ "/' -e 's/$/", 0 },/'

Used only by the pointer-coverage corpus (`corpus.json`).
