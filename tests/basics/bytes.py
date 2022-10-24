# literals
print(b'123')
print(br'123')
print(rb'123')
print(b'\u1234')

# construction
print(bytes())
print(bytes(b'abc'))

# make sure empty bytes is converted correctly
print(str(bytes(), 'utf-8'))

a = b"123"
print(a)
print(str(a))
print(repr(a))
print(a[0], a[2])
print(a[-1])
print(str(a, "utf-8"))
print(str(a, "utf-8", "strict"))
try:
    str(a, "utf-8", "strict", "toomuch")
except TypeError:
    print("TypeError")

s = 0
for i in a:
    s += i
print(s)


print(bytes(3))

print(bytes([3, 2, 1]))
print(bytes(range(5)))

# Make sure bytes are not mistreated as unicode
x = b"\xff\x8e\xfe}\xfd\x7f"
print(len(x))
print(x[0], x[1], x[2], x[3])

# Make sure init values are not mistreated as unicode chars
# For sequence of known len
print(bytes([128, 255]))
# For sequence of unknown len
print(bytes(iter([128, 255])))

# Shouldn't be able to make bytes with negative length
try:
    bytes(-1)
except ValueError:
    print('ValueError')

# Test that invalid encoding names are caught
try:
    str(a, "cabbage", "strict")
except Exception:
    print("Exception")

# Test that invalid error values are caught. On Python3 requires that an
# encoding error is actually encountered; on CP, it errors prompty at the
# time of the call
try:
    str("\x80", "utf-8", "banana")
except Exception:
    print("Exception")
