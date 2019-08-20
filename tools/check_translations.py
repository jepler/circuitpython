# Validate that all entries in the .pot are in every .po. Only the .pot is updated so we can detect
# if a translation was added to the source but isn't in a .po. This ensures translators can grab
# complete files to work on.

import sys
import polib


template_filename = sys.argv[1]
po_filenames = sys.argv[2:]

template = polib.pofile(template_filename)
all_ids = set([x.msgid for x in template])
for po_filename in po_filenames:
    print("Checking", po_filename)
    po_file = polib.pofile(po_filename)
    po_ids = set([x.msgid for x in po_file])

    missing = all_ids - po_ids
    max_print = 10
    if missing:
        print("Missing message id(s). Please run `make translate`")
        for i, m in zip(range(max_print), missing):
            print("Missing: %r" % m)
        if len(missing) > max_print:
            print("And %d more" % (len(missing) - max_print))
        sys.exit(-1)
    else:
        print("ok")
