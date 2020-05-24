"""
Process raw qstr file and output qstr data with length, hash and data bytes.

This script works with Python 2.6, 2.7, 3.3 and 3.4.
"""

from __future__ import print_function

import collections
import re
import sys
import functools

import gettext
import os.path

py = os.path.dirname(sys.argv[0])
top = os.path.dirname(py)

sys.path.append(os.path.join(top, "tools/huffman"))

import huffman

# Python 2/3 compatibility:
#   - iterating through bytes is different
#   - codepoint2name lives in a different module
import platform
if platform.python_version_tuple()[0] == '2':
    bytes_cons = lambda val, enc=None: bytearray(val)
    from htmlentitydefs import codepoint2name
elif platform.python_version_tuple()[0] == '3':
    bytes_cons = bytes
    from html.entities import codepoint2name
# end compatibility code

codepoint2name[ord('-')] = 'hyphen';

# add some custom names to map characters that aren't in HTML
codepoint2name[ord(' ')] = 'space'
codepoint2name[ord('\'')] = 'squot'
codepoint2name[ord(',')] = 'comma'
codepoint2name[ord('.')] = 'dot'
codepoint2name[ord(':')] = 'colon'
codepoint2name[ord(';')] = 'semicolon'
codepoint2name[ord('/')] = 'slash'
codepoint2name[ord('%')] = 'percent'
codepoint2name[ord('#')] = 'hash'
codepoint2name[ord('(')] = 'paren_open'
codepoint2name[ord(')')] = 'paren_close'
codepoint2name[ord('[')] = 'bracket_open'
codepoint2name[ord(']')] = 'bracket_close'
codepoint2name[ord('{')] = 'brace_open'
codepoint2name[ord('}')] = 'brace_close'
codepoint2name[ord('*')] = 'star'
codepoint2name[ord('!')] = 'bang'
codepoint2name[ord('\\')] = 'backslash'
codepoint2name[ord('+')] = 'plus'
codepoint2name[ord('$')] = 'dollar'
codepoint2name[ord('=')] = 'equals'
codepoint2name[ord('?')] = 'question'
codepoint2name[ord('@')] = 'at_sign'
codepoint2name[ord('^')] = 'caret'
codepoint2name[ord('|')] = 'pipe'
codepoint2name[ord('~')] = 'tilde'

C_ESCAPES = {
    "\a": "\\a",
    "\b": "\\b",
    "\f": "\\f",
    "\n": "\\n",
    "\r": "\\r",
    "\t": "\\t",
    "\v": "\\v",
    "\'": "\\'",
    "\"": "\\\""
}

# this must match the equivalent function in qstr.c
def compute_hash(qstr, bytes_hash):
    hash = 5381
    for b in qstr:
        hash = (hash * 33) ^ b
    # Make sure that valid hash is never zero, zero means "hash not computed"
    return (hash & ((1 << (8 * bytes_hash)) - 1)) or 1

def translate(translation_file, i18ns):
    with open(translation_file, "rb") as f:
        table = gettext.GNUTranslations(f)

        translations = []
        for original in i18ns:
            unescaped = original
            for s in C_ESCAPES:
                unescaped = unescaped.replace(C_ESCAPES[s], s)
            translation = table.gettext(unescaped)
            # Add in carriage returns to work in terminals
            translation = translation.replace("\n", "\r\n")
            translations.append((original, translation))
        return translations

def frequent_ngrams(corpus, sz, n):
    return collections.Counter(corpus[i:i+sz] for i in range(len(corpus)-sz)).most_common(n)

def max_overlap(translation, ngrams, maxmax):
    for sz in range(maxmax, 1, -1):
        for i in range(0, len(translation) - sz):
            needle = translation[i:i+sz]
            idx = ngrams.find(needle)
            if idx != -1:
                return idx, len(needle), needle
    return 0, 0, ''

def ngrams_to_pua_9(translation, ngrams):
    while True:
        idx, sz, needle = max_overlap(translation, ngrams, 9)
        if sz > 1:
            sz = min(sz, 9)
            translation = translation.replace(needle, chr(0x100 + ((sz-2) << 5) + idx))
        else:
            break
    return translation

def pua_to_ngrams_9(compressed, ngrams):
    def decode_one_element(c):
        co = ord(c)
        if co < 0x100:
            return c
        idx = co & 0x1f
        sz = 2 + ((co & 0xe0) >> 5)
        return ngrams[idx:idx+sz]

    return "".join(decode_one_element(c) for c in compressed)

def ngrams_to_pua_16(translation, ngrams):
    while True:
        idx, sz, needle = max_overlap(translation, ngrams, 33)
        if sz > 1:
            sz = min(sz, 33)
            translation = translation.replace(needle, chr(0xe000 + ((sz-2) << 8) + idx))
        else:
            break
    return translation

def pua_to_ngrams_16(compressed, ngrams):
    def decode_one_element(c):
        co = ord(c)
        if co < 0xe000 or co > 0xf8ff:
            return c
        idx = co & 0xff
        sz = 2 + ((co & 0x1f00) >> 8)
        return ngrams[idx:idx+sz]

    return "".join(decode_one_element(c) for c in compressed)

def ngrams_to_pua(translation, ngrams, mode):
    if mode == 0:
        return translation
    elif mode == 9:
        return ngrams_to_pua_9(translation, ngrams)
    else:
        return ngrams_to_pua_16(translation, ngrams)
        
class EncodingData:
    def __init__(self, lengths, values, ngramdata, huffman_data_size):
        values_type = "uint16_t" if max(ord(u) for u in values) > 255 else "uint8_t"
        maxord = max(ord(u) for u in values) 
        values_size = 1 if maxord < 512 else 2
        values9 = values_size == 1 and maxord > 255
        ngrams_type = "uint16_t" if ngramdata and max(ord(u) for u in "".join(ngramdata)) > 255 else "uint8_t"
        ngrams_size = 2 if ngramdata and max(ord(u) for u in ngramdata) > 255 else 1
        est_size = len(lengths) + values_size * len(values) + ngrams_size * len(ngramdata) + huffman_data_size + values9 * ((len(values) + 7) // 8) 
#        if ngramdata:
#            est_size += 100
#            if values9:
#                est_size += 16
        print(len(lengths) , values_size * len(values) , ngrams_size * len(ngramdata) , huffman_data_size , values9 * ((len(values) + 7) // 8), file=sys.stderr)
        self.lengths = lengths
        self.values_type = values_type
        self.values9 = values9
        self.values_size = values_size
        self.values = values
        self.ngrams_type = ngrams_type
        self.ngrams_size = ngrams_size
        self.ngramdata = ngramdata
        self.est_size = est_size

    def __lt__(self, other):
        return self.est_size < other.est_size

    def print(self, file=sys.stdout):
        print("// estimated total memory size", self.est_size, file=file)
        print("// ngram memory size", self.ngrams_size, file=file)
        print("// value memory size", self.values_size, file=file)
        print("// values", self.values, self.values9, "lengths", len(self.lengths), self.lengths, file=file)
        print("//", repr(self.ngramdata), file=file)

    def write(self, f):
        f.write("const uint8_t lengths[] = {{ {} }};\n".format(", ".join(map(str, self.lengths))))
        if self.values9:
            f.write("const {} values[] = {{ {} }};\n".format(self.values_type, ", ".join(str(ord(u) & 0xff) for u in self.values)))
            bits = sum(bool(ord(u) & 100) << i for i, u in enumerate(self.values))
            f.write("const uint8_t values9[] = {{ {} }};\n".format(", ".join(str((bits >> (i*8)) & 0xff) for i in range((len(self.values) + 7) // 8))))
        else:
            f.write("const {} values[] = {{ {} }};\n".format(self.values_type, ", ".join(str(ord(u)) for u in self.values)))
        f.write("#define ENCODING_TYPE ({})\n".format(9 if self.values9 else 2 * self.values_size))
        if self.ngramdata:
            f.write("const {} ngrams[] = {{ {} }};\n".format(self.ngrams_type, ", ".join(str(ord(u)) for u in self.ngramdata)))
            f.write("#define HAS_DICTIONARY (1)\n")
        else:
            f.write("#define HAS_DICTIONARY (0)\n".format("true" if self.ngramdata else "false"))

    def ngrams_to_pua(self, translated):
        if self.values9:
            return ngrams_to_pua_9(translated, self.ngramdata)
        return ngrams_to_pua_16(translated, self.ngramdata)

    def pua_to_ngrams(self, compressed):
        if self.values9:
            return pua_to_ngrams_9(compressed, self.ngramdata)
        return pua_to_ngrams_16(compressed, self.ngramdata)

def overlap(s1, s2):
    minlen = min(len(s1), len(s2))
    for i in range(minlen, 0, -1):
        if s1[:i] == s2[-i:]:
            return i, s1, s2
    return 0, '', ''

def find_overlap_start(forest, ngram):
    if not forest: return 0, ''
    p, q, r = max(overlap(fi, ngram) for fi in forest)
    return p, q

def find_overlap_end(forest, ngram):
    if not forest: return 0, ''
    p, q, r = max(overlap(ngram, fi) for fi in forest)
    return p, r

def best_ngram(corpus):
    histogram = collections.Counter()
    for message in corpus.split('\0'):
        for i in range(len(message) - 2):
            for j in range(i+2, len(message)):
                histogram[message[i:j]] += 1

    histogram = [(k, v * len(k)) for k, v in histogram.items() if v > 1]
    if histogram:
        histogram.sort(key=lambda x: x[1])
        return histogram.pop()[0]
    return ''

def find_best_dictionary(corpus, sz=64):
    forest = []
    corpus = "\0".join(corpus)
    if sz == 0:
        return 0, ''
    if max(ord(c) for c in corpus) <= 255 and sz <= 32:
        mode = 9
    else:
        mode = 16
    1 + sz
    while sum(len(i) for i in forest) < sz:
        ngram = best_ngram(corpus)
        if ngram == '':
            break

        if any(ngram in fi for fi in forest):
            # It could already be formed
            corpus = corpus.replace(ngram, '\0')
            continue
            
        max_overlap_start, needle_start = find_overlap_start(forest, ngram)
        max_overlap_end, needle_end = find_overlap_end(forest, ngram)

        if max_overlap_start + max_overlap_end == len(ngram) and needle_start != needle_end:
            # We can merge two items in the forest into one, and we get
            # this ngram as a result
            forest.remove(needle_start)
            forest.remove(needle_end)
            forest.append(needle_start + needle_end)
        elif max_overlap_start == max_overlap_end == 0:
            # Doesn't overlap with anything in the forest
            if len(ngram) + sum(len(fi) for fi in forest) < sz:
                forest.append(ngram)
        elif max_overlap_start > max_overlap_end:
            if len(ngram) - max_overlap_start + sum(len(fi) for fi in forest) < sz:
                forest.remove(needle_start)
                forest.append(needle_start + ngram[max_overlap_start:])
        else:
            # max_overlap_end >= max_overlap_start
            if len(ngram) - max_overlap_end + sum(len(fi) for fi in forest) < sz:
                forest.remove(needle_end)
                forest.append(ngram[max_overlap_end:] + needle_end)
        corpus = corpus.replace(ngram, '\0')
    return mode, "".join(forest)

def compute_huffman_coding(translations, qstrs, dict_sz):
    all_strings = [x[1] for x in translations]

    # go through each qstr and print it out
    for _, _, qstr in qstrs.values():
        all_strings.append(qstr)

    all_strings_concat = "".join(all_strings)
    mode, ngramsdata = find_best_dictionary(all_strings, dict_sz)
    all_strings_concat = "".join(ngrams_to_pua(s, ngramsdata, mode) for s in all_strings)
    counts = collections.Counter(all_strings_concat)
    cb = huffman.codebook(counts.items())
    values = []
    length_count = {}
    renumbered = 0
    last_l = None
    canonical = {}
    for ch, code in sorted(cb.items(), key=lambda x: (len(x[1]), x[0])):
        values.append(ch)
        l = len(code)
        if l not in length_count:
            length_count[l] = 0
        length_count[l] += 1
        if last_l:
            renumbered <<= (l - last_l)
        canonical[ch] = '{0:0{width}b}'.format(renumbered, width=l)
        s = C_ESCAPES.get(ch, ch)
        #print("//", ord(ch), s, counts[ch], canonical[ch], renumbered)
        renumbered += 1
        last_l = l
    lengths = bytearray()
    for i in range(1, max(length_count) + 2):
        lengths.append(length_count.get(i, 0))
    ngramdata = [ord(i) for i in ngramsdata]
    huffman_data_size = sum((len(cb[u]) + 7)//8 for u in all_strings_concat)
    result = EncodingData(lengths, values, ngramsdata, huffman_data_size)
    result.print(file=sys.stderr)
    return result

def decompress(encoder, encoded, length):
    values = encoder.values
    lengths = encoder.lengths
    #print(l, encoded)
    dec = []
    this_byte = 0
    this_bit = 7
    b = encoded[this_byte]
    for i in range(length):
        bits = 0
        bit_length = 0
        max_code = lengths[0]
        searched_length = lengths[0]
        while True:
            bits <<= 1
            if 0x80 & b:
                bits |= 1

            b <<= 1
            bit_length += 1
            if this_bit == 0:
                this_bit = 7
                this_byte += 1
                if this_byte < len(encoded):
                    b = encoded[this_byte]
            else:
                this_bit -= 1
            if max_code > 0 and bits < max_code:
                #print('{0:0{width}b}'.format(bits, width=bit_length))
                break
            max_code = (max_code << 1) + lengths[bit_length]
            searched_length += lengths[bit_length]

        v = values[searched_length + bits - max_code]
        dec.append(v)
    return encoder.pua_to_ngrams(''.join(dec))

def compress(encoder, decompressed):
    if not isinstance(decompressed, str):
        raise TypeError()
    values = encoder.values
    lengths = encoder.lengths
    enc = bytearray(len(decompressed) * 3)
    #print(decompressed)
    #print(lengths)
    current_bit = 7
    current_byte = 0
    for c in decompressed:
        #print()
        #print("char", c, values.index(c))
        start = 0
        end = lengths[0]
        bits = 1
        compressed = None
        code = 0
        while compressed is None:
            s = start
            e = end
            #print("{0:0{width}b}".format(code, width=bits))
            # Binary search!
            while e > s:
                midpoint = (s + e) // 2
                #print(s, e, midpoint)
                if values[midpoint] == c:
                    compressed = code + (midpoint - start)
                    #print("found {0:0{width}b}".format(compressed, width=bits))
                    break
                elif c < values[midpoint]:
                    e = midpoint
                else:
                    s = midpoint + 1
            code += end - start
            code <<= 1
            start = end
            end += lengths[bits]
            bits += 1
            #print("next bit", bits)

        for i in range(bits - 1, 0, -1):
            if compressed & (1 << (i - 1)):
                enc[current_byte] |= 1 << current_bit
            if current_bit == 0:
                current_bit = 7
                #print("packed {0:0{width}b}".format(enc[current_byte], width=8))
                current_byte += 1
            else:
                current_bit -= 1
    if current_bit != 7:
        current_byte += 1
    if current_byte > len(decompressed):
        print("Note: compression increased length", repr(decompressed), len(decompressed), current_byte, file=sys.stderr)
    return enc[:current_byte]

def qstr_escape(qst):
    def esc_char(m):
        c = ord(m.group(0))
        try:
            name = codepoint2name[c]
        except KeyError:
            name = '0x%02x' % c
        return "_" + name + '_'
    return re.sub(r'[^A-Za-z0-9_]', esc_char, qst)

def parse_input_headers(infiles):
    # read the qstrs in from the input files
    qcfgs = {}
    qstrs = {}
    i18ns = set()
    for infile in infiles:
        with open(infile, 'rt') as f:
            for line in f:
                line = line.strip()

                # is this a config line?
                match = re.match(r'^QCFG\((.+), (.+)\)', line)
                if match:
                    value = match.group(2)
                    if value[0] == '(' and value[-1] == ')':
                        # strip parenthesis from config value
                        value = value[1:-1]
                    qcfgs[match.group(1)] = value
                    continue


                match = re.match(r'^TRANSLATE\("(.*)"\)$', line)
                if match:
                    i18ns.add(match.group(1))
                    continue

                # is this a QSTR line?
                match = re.match(r'^Q\((.*)\)$', line)
                if not match:
                    continue

                # get the qstr value
                qstr = match.group(1)

                # special case to specify control characters
                if qstr == '\\n':
                    qstr = '\n'

                # work out the corresponding qstr name
                ident = qstr_escape(qstr)

                # don't add duplicates
                if ident in qstrs:
                    continue

                # add the qstr to the list, with order number to retain original order in file
                order = len(qstrs)
                # but put special method names like __add__ at the top of list, so
                # that their id's fit into a byte
                if ident == "":
                    # Sort empty qstr above all still
                    order = -200000
                elif ident == "__dir__":
                    # Put __dir__ after empty qstr for builtin dir() to work
                    order = -190000
                elif ident.startswith("__"):
                    order -= 100000
                qstrs[ident] = (order, ident, qstr)

    if not qcfgs and qstrs:
        sys.stderr.write("ERROR: Empty preprocessor output - check for errors above\n")
        sys.exit(1)

    return qcfgs, qstrs, i18ns

def make_bytes(cfg_bytes_len, cfg_bytes_hash, qstr):
    qbytes = bytes_cons(qstr, 'utf8')
    qlen = len(qbytes)
    qhash = compute_hash(qbytes, cfg_bytes_hash)
    if all(32 <= ord(c) <= 126 and c != '\\' and c != '"' for c in qstr):
        # qstr is all printable ASCII so render it as-is (for easier debugging)
        qdata = qstr
    else:
        # qstr contains non-printable codes so render entire thing as hex pairs
        qdata = ''.join(('\\x%02x' % b) for b in qbytes)
    if qlen >= (1 << (8 * cfg_bytes_len)):
        print('qstr is too long:', qstr)
        assert False
    qlen_str = ('\\x%02x' * cfg_bytes_len) % tuple(((qlen >> (8 * i)) & 0xff) for i in range(cfg_bytes_len))
    qhash_str = ('\\x%02x' * cfg_bytes_hash) % tuple(((qhash >> (8 * i)) & 0xff) for i in range(cfg_bytes_hash))
    return '(const byte*)"%s%s" "%s"' % (qhash_str, qlen_str, qdata)

def print_qstr_data(encoder, qcfgs, qstrs, i18ns):
    # get config variables
    cfg_bytes_len = int(qcfgs['BYTES_IN_LEN'])
    cfg_bytes_hash = int(qcfgs['BYTES_IN_HASH'])

    # print out the starter of the generated C header file
    print('// This file was automatically generated by makeqstrdata.py')
    print('')

    # add NULL qstr with no hash or data
    print('QDEF(MP_QSTR_NULL, (const byte*)"%s%s" "")' % ('\\x00' * cfg_bytes_hash, '\\x00' * cfg_bytes_len))

    total_qstr_size = 0
    total_qstr_compressed_size = 0
    # go through each qstr and print it out
    for order, ident, qstr in sorted(qstrs.values(), key=lambda x: x[0]):
        qbytes = make_bytes(cfg_bytes_len, cfg_bytes_hash, qstr)
        print('QDEF(MP_QSTR_%s, %s)' % (ident, qbytes))
        total_qstr_size += len(qstr)

    total_text_size = 0
    total_text_compressed_size = 0
    for original, translation in i18ns:
        translation_encoded = translation.encode("utf-8")
        translation_ngrammed = encoder.ngrams_to_pua(translation)
        compressed = compress(encoder, translation_ngrammed)
        total_text_compressed_size += len(compressed)
        decompressed = decompress(encoder, compressed, len(translation_ngrammed))
        assert decompressed == translation
        for c in C_ESCAPES:
            decompressed = decompressed.replace(c, C_ESCAPES[c])
        print("TRANSLATION(\"{}\", {}, {{ {} }}) // {}".format(original, len(translation_encoded)+1, ", ".join(["0x{:02x}".format(x) for x in compressed]), decompressed))
        total_text_size += len(translation.encode("utf-8"))

    print()
    print("// {} bytes worth of qstr".format(total_qstr_size))
    print("// {} bytes worth of translations".format(total_text_size))
    print("// {} bytes worth of translations compressed".format(total_text_compressed_size))
    print("// {} bytes saved".format(total_text_size - total_text_compressed_size))

def print_qstr_enums(qstrs):
    # print out the starter of the generated C header file
    print('// This file was automatically generated by makeqstrdata.py')
    print('')

    # add NULL qstr with no hash or data
    print('QENUM(MP_QSTR_NULL)')

    # go through each qstr and print it out
    for order, ident, qstr in sorted(qstrs.values(), key=lambda x: x[0]):
        print('QENUM(MP_QSTR_%s)' % (ident,))

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description='Process QSTR definitions into headers for compilation')
    parser.add_argument('infiles', metavar='N', type=str, nargs='+',
                        help='an integer for the accumulator')
    parser.add_argument('--translation', default=None, type=str,
                        help='translations for i18n() items')
    parser.add_argument('--compression_filename', default=None, type=str,
                        help='header for compression info')

    args = parser.parse_args()

    qcfgs, qstrs, i18ns = parse_input_headers(args.infiles)
    if args.translation:
        translations = translate(args.translation, i18ns)
        dictionary_sizes = [32]
        best_encoding = min(compute_huffman_coding(translations, qstrs, i) for i in dictionary_sizes)
        best_encoding.print()
        with open(args.compression_filename, "w") as f:
            best_encoding.write(f)
        encoding_table = (best_encoding.values, best_encoding.lengths)
        ngrams = best_encoding.ngramdata
        print_qstr_data(best_encoding, qcfgs, qstrs, translations)
    else:
        print_qstr_enums(qstrs)
