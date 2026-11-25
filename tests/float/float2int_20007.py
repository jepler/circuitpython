# reproducer for #20007
# requires: float

print("small positive number:", 1e-40)
print("int conversion:", int(1e-40))

print("small negative number:", -1e-40)
print("int conversion:", int(-1e-40))  # ValueError: float too big
