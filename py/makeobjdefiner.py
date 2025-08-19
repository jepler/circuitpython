for i in range(13):
    r = range(i)
    filled_slots = "|".join(f"slot_weight_##f{j}" for j in r) if i else "0"
    slots = "".join(
        f"\n            [MP_OBJ_FIND_SLOT_STATIC(f{j}, {filled_slots})] = v{j}, \\" for j in r
    )
    print(f"""\
#define MP_DEFINE_CONST_OBJ_TYPE_NARGS_{i}(_struct_type, _typename, _name, _flags{"".join(f", f{j}, v{j}" for j in r)}) \\
    const _struct_type _typename = {{ \\
        .base = {{ &mp_type_type }}, \\
        .flags = _flags, \\
        .name = _name, \\
        .filled_slots = {filled_slots}, \\
        .slots = {{ \\{slots}
        }}, \\
    }}""")
