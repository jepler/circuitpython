# test if eval raises SyntaxError

try:
    eval
except NameError:
    print("SKIP")
    raise SystemExit

try:
    print(eval("[1,,]"))
except SyntaxError:
    print("SyntaxError")

try:
    print(eval("123\\"))
except SyntaxError:
    print("SyntaxError")

try:
    print(eval("123\\\n"))
except SyntaxError:
    print("SyntaxError")
