import pathlib
import sys
import subprocess
import textwrap
import yaml
import dataclasses
import click
from functools import singledispatchmethod
from dataclasses import dataclass, Field
from typing import Any, get_args, get_origin, Union

script_dir = pathlib.Path(__file__).parent

if sys.version_info >= (3, 10):
    from types import UnionType
else:
    UnionType = type(Union[int, float])

p = pathlib.Path("etc/needs-glue.txt")
if p.exists():
    needs_glue = set(p.read_text().split("\n"))
else:
    needs_glue = set()


typecodes = {
    'bool': 'B',
    'char': 'B',
    'uint8_t': 'B',
    'uint16_t': 'H',
    'uint32_t': 'L',
    'uint64_t': 'Q',
    'float': 'f',
    'double': 'd',
    'int8_t': 'b',
    'int16_t': 'h',
    'int32_t': 'l',
    'int64_t': 'q',
}


@dataclass(frozen=True)
class Scalar:
    name: str
    typecode: str
    is_const: bool = False

    @property
    def is_ptr(self):
        return False

    @property
    def is_array(self):
        return False

    @property
    def is_scalar(self):
        return True


@dataclass(frozen=True)
class Ptr:
    pointee: Any
    is_const: bool = False

    @property
    def is_ptr(self):
        return True

    @property
    def is_array(self):
        return False

    @property
    def is_scalar(self):
        return False

    def __str__(self):
        return f"{self.pointee}*"


@dataclass(frozen=True)
class Array:
    pointee: Any
    bound: int
    is_const: bool = False

    @property
    def is_ptr(self):
        return False

    @property
    def is_array(self):
        return True

    @property
    def is_scalar(self):
        return False


@dataclass
class CommonFields:
    comment: str | None = None
    only_for: str | None = None
    not_for: str | None = None
    api: str | None = None
    doc: str | None = None
    emit: bool = True


def toplevel(cls):
    for f in dataclasses.fields(CommonFields):
        setattr(cls, f.name, f)
        cls.__annotations__[f.name] = CommonFields.__annotations__[f.name]
    return dataclass(cls, frozen=True)


@dataclass(frozen=True)
class StructMember:
    name: str
    type: str
    comment: str | None = None
    fulltype: any = None


@toplevel
class Union:
    name: str
    members: list[StructMember] = dataclasses.field(default_factory=list)
    size: int | None = None

    @property
    def is_ptr(self):
        return False

    @property
    def is_array(self):
        return False

    @property
    def is_scalar(self):
        return False


@toplevel
class Struct:
    name: str
    members: list[StructMember] = dataclasses.field(default_factory=list)
    size: int | None = None

    @property
    def is_ptr(self):
        return False

    @property
    def is_array(self):
        return False

    @property
    def is_scalar(self):
        return False


@dataclass
class EnumMember:
    name: str
    value: int | str | None = None  # a number or a 4 character literal
    old_name: str | None = None


@toplevel
class Enum:
    values: list[EnumMember]
    name: str | None = None


@toplevel
class Typedef:
    name: str
    type: str
    size: int | None = None  # typedef AE_hdlr_t has this .. why?
    fulltype: any = None
    is_const: bool = False


@toplevel
class LowMem:
    name: str
    type: str
    address: int


@toplevel
class PyVerbatim:
    content: str | None = None
    typedef_content: str | None = None
    name: str | None = None


@toplevel
class Verbatim:
    verbatim: str


@dataclass
class Argument:
    type: str
    name: str = ""
    register: str | None = None


@toplevel
class Function:
    name: str
    trap: int | None = None
    args: list[Argument] = dataclasses.field(default_factory=list)
    executor: str | bool | None = None
    return_: str | None = None
    inline: str | None = None
    noinline: str | None = None
    dispatcher: str | None = None
    selector: int | None = None
    returnreg: str | None = None
    old_name: str | None = None


@toplevel
class FunPtr:
    name: str
    args: list[Argument] = dataclasses.field(default_factory=list)
    return_: str | None = None


@toplevel
class Dispatcher:
    name: str
    trap: int
    selector_location: str | None = None


yaml_types = {
    'function': Function,
    'funptr': FunPtr,
    'verbatim': Verbatim,
    'pyverbatim': PyVerbatim,
    'lowmem': LowMem,
    'typedef': Typedef,
    'enum': Enum,
    'struct': Struct,
    'union': Union,
    'dispatcher': Dispatcher,
}


def fix_key(k):
    k = k.replace('-', '_')
    if k == 'return':
        k += "_"
    return k


def identify(y):
    for k in y.keys():
        if r := yaml_types.get(k):
            result = dict(y)
            if k != 'verbatim':
                # Verbatim just has a str value, it's not a namespace
                result.update(result.pop(k, {}))
            return result, r
    raise RuntimeError(f"Could not identify field type for {y!r}")


@dataclass
class OneOf:
    types: tuple[type]

    def __call__(self, value):
        for t in self.types:
            try:
                return t(value)
            except TypeError:
                continue
        else:
            raise TypeError(f"Could not convert {value} to any of {self.types}")


def get_field_type(field: Field[Any] | type) -> Any:
    if isinstance(field, type):
        return field
    field_type = field.type
    if isinstance(field_type, str):
        raise RuntimeError("parameters dataclass may not use 'from __future__ import annotations")
    origin = get_origin(field_type)
    if origin in (Union, UnionType):
        return OneOf(tuple(a for a in get_args(field_type) if a is not None))
    if origin is list:
        return [get_field_type(get_args(field_type)[0])]
    return field_type


def yaml_to_type(y, t=None):
    if t is None:
        y, t = identify(y)
    if isinstance(t, OneOf):
        if isinstance(y, t.types):
            return y
    else:
        if isinstance(y, t):
            return y
    try:
        kwargs = {}
        fields = {f.name: f for f in dataclasses.fields(t)}
        for k, v in y.items():
            k1 = fix_key(k)
            field = fields[k1]
            field_type = get_field_type(field)
            if isinstance(v, list):
                kwargs[k1] = [yaml_to_type(vi, field_type[0]) for vi in v]
            else:
                kwargs[k1] = yaml_to_type(v, field_type)
        return t(**kwargs)
    except (KeyError, TypeError, AttributeError) as e:
        raise TypeError(f"Converting node {y} to {t}") from e


def load_defs(path):
    with open(path) as f:
        defs = yaml.safe_load(f)
    return [yaml_to_type(d) for d in defs]


signed_integer_types = {'uint8_t', 'uint16_t', 'uint32_t'}
unsigned_integer_types = {'bool', 'char', 'int8_t', 'int16_t', 'int32_t'}
all_integer_types = signed_integer_types | unsigned_integer_types
all_float_types = {"float", "double"}
all_scalar_types = all_integer_types | all_float_types


@dataclass
class descr_maker_scalar:
    tag: str

    def __call__(self, emitter, offset):
        return f"MP_ROM_INT(UCTYPE_TYPE({self.tag}) | {offset})"


@dataclass
class descr_maker_struct:
    tag: str

    def __call__(self, emitter, offset):
        obj = emitter.common_definition(
            "const mp_rom_obj_tuple_t",
            f"ROM_TUPLE(MP_ROM_INT({offset}), MP_ROM_PTR((void*)&{self.tag}_obj))",
        )
        return f"MP_ROM_PTR((void*)&{obj})"


@dataclass
class descr_maker_arr_scalar:
    tag: str
    size: int

    def __call__(self, emitter, offset):
        obj = emitter.common_definition(
            "const mp_rom_obj_tuple_t",
            f"ROM_TUPLE(MP_ROM_INT({offset} | UCTYPE_AGG(ARRAY)), MP_ROM_INT(UCTYPE_TYPE({self.tag}) | {self.size}))",
        )
        return f"MP_ROM_PTR((void*)&{obj})"


@dataclass
class descr_maker_arr_struct:
    tag: str
    size: int

    def __call__(self, emitter, offset):
        obj = emitter.common_definition(
            "const mp_rom_obj_tuple_t",
            f"ROM_TUPLE(MP_ROM_INT({offset} | UCTYPE_AGG(ARRAY)), MP_ROM_INT({self.size}), MP_ROM_PTR((void*)(&{self.tag}_obj)))",
        )
        return f"MP_ROM_PTR((void*)&{obj})"


@dataclass
class descr_maker_ptr_scalar:
    tag: str

    def __call__(self, emitter, offset):
        obj = emitter.common_definition(
            "const mp_rom_obj_tuple_t",
            f"ROM_TUPLE(MP_ROM_INT({offset} | UCTYPE_AGG(PTR)), MP_ROM_INT(UCTYPE_TYPE({self.tag})))",
        )
        return f"MP_ROM_PTR((void*)&{obj})"


class descr_maker_ptr_struct:
    def __init__(self, tag):
        self.tag = tag

    def __call__(self, emitter, offset):
        obj = emitter.common_definition(
            "const mp_rom_obj_tuple_t",
            f"ROM_TUPLE(MP_ROM_INT({offset} | UCTYPE_AGG(PTR)), MP_ROM_PTR((void*)&{self.tag}_obj))",
        )
        return f"MP_ROM_PTR((void*)&{obj})"


@dataclass
class PointConverter:
    fieldname: str

    def emit_to_c(self, name_py, name_c):
        return f"Point {name_c} = Point_to_c({name_py}, MP_QSTR_{self.fieldname});\n"

    def emit_to_py(self, name_c):
        raise RuntimeError("not implemented")


converters = {
    'Point': PointConverter,
}


class ScalarConverter:
    def __init__(self, type_c, to_c, to_py):
        self.type_c = type_c
        self.to_c = to_c
        self.to_py = to_py

    def emit_decls(self):
        pass

    def emit_to_c(self, name_py, name_c):
        return f"{self.type_c} {name_c} = {self.to_c}({name_py});\n"

    def emit_to_py(self, name_c):
        return f"{self.to_py}({name_c})"

    def emit_call_arg(self, name_c):
        return name_c


@dataclass
class PtrConverter:
    emitter: object
    fieldname: str
    type_c: object
    type_obj: object
    deref: bool = False
    is_const: bool = False

    def __post_init__(self):
        assert self.type_c is not None, f"type_c must be a type description, not {self.type_c}"

    def emit_to_c(self, name_py, name_c):
        print(f"emit_to_c {name_py} {self.type_c}")
        is_const = +self.is_const  # to get 0/1, not True/False
        resolved_type = self.emitter.parse_type(self.type_c)
        if (
            isinstance(resolved_type, Ptr)
            and self.emitter.parse_type(resolved_type.pointee) not in all_scalar_types
        ):
            type_obj = f"(void*)&{resolved_type.pointee}_obj"
            return f"{self.type_c} {name_c} = to_struct_helper({name_py}, {type_obj}, {is_const}, MP_QSTR_{self.fieldname}); // 1\n"
        else:
            return f"{self.type_c} {name_c} = to_struct_helper({name_py}, {self.type_obj}, {is_const}, MP_QSTR_{self.fieldname}); // 2\n"

    def emit_to_py(self, name_c):
        print(f"emit_to_py {self.type_c}")
        resolved_type = self.emitter.parse_type(self.type_c)
        if resolved_type in self.emitter.types:
            resolved_type = self.emitter.parse_type(self.emitter.types[resolved_type])
        print("emit_to_py", self.type_c, resolved_type)
        if isinstance(resolved_type, Ptr):
            assert self.type_obj and self.type_obj != "null"
            type_str = resolved_type.pointee.replace("*", "Ptr")
            return f"from_struct_helper({name_c}, (void*)&{type_str}_obj) /* {self.type_c} 3 */"
        else:
            type_str = self.type_obj.replace("*", "Ptr")
            return f"from_struct_helper({name_c}, {self.type_obj}) /* {resolved_type} 4 */"

    def emit_call_arg(self, name_c):
        if self.deref:
            return f"*{name_c}"
        return name_c


def make_converter(emitter, fieldname, typedesc_in):
    typedesc = emitter.resolve_typedefs(typedesc_in)
    print(f"make_converter {typedesc=}")
    type_c = getattr(typedesc_in, 'name', None)
    if converter := converters.get(type_c):
        return converter(fieldname)
    if type_c in signed_integer_types:
        return ScalarConverter(typedesc, "mp_obj_get_int", "mp_obj_new_int")
    if isinstance(typedesc, FunPtr):
        return ScalarConverter(
            typedesc, f"({type_c})mp_obj_get_int_truncated", "mp_obj_new_int_from_ptr"
        )
    if typedesc in unsigned_integer_types:
        return ScalarConverter(typedesc, "mp_obj_get_int_truncated", "mp_obj_new_int_from_uint")
    if type_c == "void*":
        return ScalarConverter(typedesc, "void_ptr_from_py", "mp_obj_new_int_from_uint")
    if typedesc.is_ptr:
        base_type = emitter.resolve_typedefs(typedesc.pointee)
        if base_type in emitter.types:
            return PtrConverter(
                emitter,
                fieldname,
                type_c,
                f"(const mp_obj_type_t*)&{base_type.name}_obj",
                is_const=typedesc.is_const,
            )
        if base_type != 'void':
            emitter.info.append(f"confused about {typedesc} from {typedesc_in}")
        return PtrConverter(emitter, fieldname, typedesc_in, "NULL", is_const=typedesc.is_const)
    print(f"note: {emitter.funptrs=}")
    raise ValueError(f"no converter possible for {type_c} ({resolved_type=})")


class Processor:
    def __init__(self, modname):
        self.modname = modname
        self.decls = []
        self.body = []
        self.locals = []
        self.definitions = {}
        self.info = []
        self.unknowns = set()
        self.types = {k: Scalar(k, v) for k, v in typecodes.items()}
        self.typedef_objs = set()
        self.structs = {}
        self.funptrs = set(("ProcPtr",))
        self.decls_dedent("""
        #include "extmod/multiverse_support.h"

        """)
        self.add_local("__name__", f"MP_ROM_QSTR(MP_QSTR_{self.modname})")

    def common_definition(self, c_type: str, c_value: str) -> str:
        k = (c_type, c_value)
        if k not in self.definitions:
            i = len(self.definitions)
            self.definitions[k] = i
            self.decls_dedent(f"static {c_type} common_{i} = {c_value};")
        return f"common_{self.definitions[k]}"

    def typestr_is_array(self, typestr):
        if typestr.count(']') + ("*" in typestr) > 1:
            raise ValueError(
                f"array-of-array or pointer-to-array or array-of-pointers NYI {typestr}"
            )
        return typestr.endswith("]")

    def typestr_remove_array(self, typestr):
        return typestr.partition("[")[0]

    def typestr_remove_ptr(self, typestr):
        return typestr.removesuffix("*")

    def typestr_is_const(self, typestr):
        return typestr.startswith("const ")

    def typestr_remove_const(self, typestr):
        return typestr.removeprefix("const ")

    def typestr_array_size(self, typestr):
        return int(typestr.partition("[")[2].removesuffix("]"))

    def typestr_is_ptr(self, typestr):
        return typestr.endswith("*")

    def typestr_is_scalar(self, typestr):
        return typestr in all_scalar_types

    def typestr_is_array(self, typestr):
        return "[" in typestr

    def decls_dedent(self, text):
        self.decls.append(textwrap.dedent(text.rstrip()))

    def body_dedent(self, text):
        self.body.append(textwrap.dedent(text.rstrip()))

    def typestr_parse(self, typestr):
        is_const = self.typestr_is_const(typestr)
        typestr = self.typestr_remove_const(typestr)
        if self.typestr_is_array(typestr):
            bound = self.typestr_array_size(typestr)
            pointee = self.types[self.typestr_remove_array(typestr)]
            return Array(pointee, bound, is_const=is_const)
        elif self.typestr_is_ptr(typestr):
            pointee = self.types[self.typestr_remove_ptr(typestr)]
            return Ptr(pointee, is_const=is_const)
        base_type = self.resolve_typedefs(typestr)
        return dataclasses.replace(base_type, is_const=is_const)

    def resolve_typedefs(self, typedesc):
        typedesc_in = typedesc
        if isinstance(typedesc, str):
            typedesc = self.types[typedesc]
        while isinstance(typedesc, Typedef):
            typedesc = typedesc.fulltype
        assert typedesc.is_scalar or not typedesc.is_scalar
        return typedesc

    def typedefs(self, defs):
        for d in defs:
            if isinstance(d, Typedef):
                d = dataclasses.replace(d, fulltype=self.typestr_parse(d.type))
                print("typedef", d.name, self.typestr_parse(d.type))
                assert d.fulltype is not None
                self.types[d.name] = d
                self.decls_dedent(f"MP_DECLARE_CTYPES_STRUCT({d.name}_obj); // typedef")
            if isinstance(d, Struct) and d.members:
                d = dataclasses.replace(
                    d,
                    members=tuple(
                        dataclasses.replace(m, fulltype=self.types[m.type]) for m in d.members
                    ),
                )
                self.structs[d.name] = d
                self.types[d.name] = d
                self.decls_dedent(f"MP_DECLARE_CTYPES_STRUCT({d.name}_obj); // struct")
            if isinstance(d, Union) and d.members:
                d = dataclasses.replace(
                    d,
                    members=tuple(
                        dataclasses.replace(m, fulltype=self.types[m.type]) for m in d.members
                    ),
                )
                self.structs[d.name] = d
                self.types[d.name] = d
                self.decls_dedent(f"MP_DECLARE_CTYPES_STRUCT({d.name}_obj); // union")
            if isinstance(d, PyVerbatim) and d.typedef_content:
                self.decls_dedent(d.typedef_content)
            if isinstance(d, FunPtr):
                self.funptrs.add(d.name)
                slf.types[d.name] = d
        print(self.types)

    def emit(self, defs):
        for d in defs:
            print("emit", id(d), d)
            d = self.types.get(getattr(d, 'name', None), d)
            try:
                self.emit_node(d)
            except Exception as e:
                e.add_note(f"While converting {d}")
                raise

    @singledispatchmethod
    def emit_node(self, node):
        if type(node) in self.unknowns:
            return
        self.unknowns.add(type(node))
        raise RuntimeError(f"# Unknown {node!r:.68s}...")

    @emit_node.register
    def emit_dispatcher(self, dispatcher: Dispatcher):
        pass  # Nothing to emit

    @emit_node.register
    def emit_typedef(self, typedef: Typedef):
        print("emit_typedef", typedef)
        self.body_dedent(f"// typedef {typedef}")
        name = typedef.name
        fulltype = self.resolve_typedefs(typedef.fulltype)
        if fulltype is None:
            raise RuntimeError(typedef)
        if fulltype.is_ptr or fulltype.is_array:
            make_descr = self.descr_maker(typedef.fulltype)
            self.body_dedent(f"// {type} {make_descr=}")
            if make_descr is None:
                self.body_dedent(f"// {typedef}: no make_descr")
                return
            offset = 0
            self.body_dedent(f"""
            MP_DEFINE_CTYPES_STRUCT({name}_obj, MP_QSTR_{name}, {make_descr(self, offset)}, LAYOUT_NATIVE); // 1
            """)
            self.typedef_objs.add(name)
            self.add_local(name)
            offset = 0
        else:
            self.add_local_alias(name, typedef.name)
            self.body_dedent(f"// Just an alias: {typedef}")

    def descr_maker(self, typedesc):
        typedesc = self.resolve_typedefs(typedesc)
        if isinstance(typedesc, FunPtr):
            return None
        print("descr_maker", typedesc)
        is_ptr = typedesc.is_ptr
        is_array = typedesc.is_array
        if is_ptr or is_array:
            basetype = typedesc.pointee
        else:
            basetype = typedesc

        basetype = self.resolve_typedefs(basetype)

        if basetype.is_scalar:
            basetypename = basetype.name
            u = "U" if basetypename.startswith("u") else ""
            if basetypename == "bool":
                type_str = "UINT8"
            elif basetypename == "char":
                type_str = "INT8"
            elif basetypename == "double":
                type_str = "FLOAT64"
            elif basetypename == "float":
                type_str = "FLOAT32"
            elif basetypename.endswith(("32", "32_t")):
                type_str = f"{u}INT32"
            elif basetypename.endswith(("16", "16_t")):
                type_str = f"{u}INT16"
            elif basetypename.endswith(("8", "8_t")):
                type_str = f"{u}INT8"
            else:
                raise RuntimeError(f"teach me about {basetypename}")

            if is_ptr:
                return descr_maker_ptr_scalar(type_str)
            if is_array:
                return descr_maker_arr_scalar(type_str, typedesc.bound)
            return descr_maker_scalar(type_str)
        if is_ptr:
            if basetype in self.types:
                return descr_maker_ptr_struct(basetypename)
            return descr_maker_ptr_scalar("UINT8")
        if is_array:
            basetypename = basetype.name
            return descr_maker_arr_struct(basetypename, fulltype.bound)
            basetypename = basetype.pointee.name
        return descr_maker_struct(basetype.name)

        return descr

    def struct_make_table(self, struct):
        rows = []
        for member in struct.members:
            make_descr = self.descr_maker(member.fulltype)
            if make_descr is None:
                continue
            offset = f"offsetof({struct.name}, {member.name})"
            rows.append(f"{{ MP_ROM_QSTR(MP_QSTR_{member.name}), {make_descr(self, offset)} }},")
        return "\n".join(rows)

    def union_make_table(self, union):
        rows = []
        for member in union.members:
            make_descr = self.descr_maker(member.fulltype)
            if make_descr is None:
                continue
            rows.append(f"{{ MP_ROM_QSTR(MP_QSTR_{member.name}), {make_descr(self, 0)} }},")
        return "\n".join(rows)

    @emit_node.register
    def emit_union(self, e: Union):
        name = e.name
        self.body_dedent(f"""
        static const mp_rom_map_elem_t {name}_descr_table[] = {{
             {self.union_make_table(e)}
        }};
        static MP_DEFINE_CONST_DICT({name}_descr_dict, {name}_descr_table);
        MP_DEFINE_CTYPES_STRUCT({name}_obj, MP_QSTR_{name}, MP_ROM_PTR((void*)&{name}_descr_dict), LAYOUT_NATIVE); // 2
        """)
        self.add_local(name)

    @emit_node.register
    def emit_struct(self, e: Struct):
        name = e.name
        ee = self.structs.get(e.name)
        if ee and (e is not ee):
            print(f"omitting {e} in favor of {ee}")
            return  # better defn available
        self.body_dedent(f"""
        static const mp_rom_map_elem_t {name}_descr_table[] = {{
             {self.struct_make_table(e)}
        }};
        static MP_DEFINE_CONST_DICT({name}_descr_dict, {name}_descr_table);
        MP_DEFINE_CTYPES_STRUCT({name}_obj, MP_QSTR_{name}, MP_ROM_PTR((void*)&{name}_descr_dict), LAYOUT_NATIVE); // 3
        """)
        self.add_local(name)

    @emit_node.register
    def emit_enum(self, e: Enum):
        for v in e.values:
            if v.value is not None and v.name is not None:
                self.locals.append(f"{{ MP_ROM_QSTR(MP_QSTR_{v.name}), MP_ROM_INT({v.value}) }},")
            # else:
            # self.info.append(f"enumerant without value: {v['name']}")

    @emit_node.register
    def emit_lowmem(self, lm: LowMem):
        name = lm.name
        address = lm.address
        typename = lm.type
        self.body_dedent(f"""
            static mp_obj_t LMGet{name}_fn(size_t n_args, const mp_obj_t *args) {{
                return LMGet_common({address}, sizeof({typename}), n_args == 0 ? mp_const_none : args[0]);
            }}
            MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(LMGet{name}_obj, 0, 1, LMGet{name}_fn);

            static mp_obj_t LMSet{name}_fn(mp_obj_t value) {{
                LMSet_common({address}, sizeof({typename}), value);
                return mp_const_none;
            }}
            MP_DEFINE_CONST_FUN_OBJ_1(LMSet{name}_obj, LMSet{name}_fn);
            """)
        self.add_local(f"LMGet{name}")
        self.add_local(f"LMSet{name}")

    def add_local(self, name, value=...):
        if value is ...:
            value = f"MP_ROM_PTR((void*)&{name}_obj)"
        self.locals.append(f"{{ MP_ROM_QSTR(MP_QSTR_{name}), {value} }},")

    def add_local_alias(self, name, alias):
        value = f"MP_ROM_PTR((void*)&{alias}_obj)"
        return self.add_local(name, value)

    @emit_node.register
    def emit_verbatim(self, v: Verbatim):
        pass  # Ignore C verbatim blocks

    @emit_node.register
    def emit_pyverbatim(self, v: PyVerbatim):
        if v.content:
            self.body.append(v.content)
        if v.name is not None:
            self.add_local(v.name)

    # {'args': [{'name': 'src_bitmap', 'type': 'BitMap*'},
    #          {'name': 'dst_bitmap', 'type': 'BitMap*'},
    #          {'name': 'src_rect', 'type': 'const Rect*'},
    #          {'name': 'dst_rect', 'type': 'const Rect*'},
    #          {'name': 'mode', 'type': 'INTEGER'},
    #          {'name': 'mask', 'type': 'RgnHandle'}],
    # 'executor': 'C_',
    # 'name': 'CopyBits',
    # 'trap': 43244}

    def fun_declare_args_enum(self, args):
        if args:
            argnames = [arg.name or f"arg{i}" for i, arg in enumerate(args)]
            args = ", ".join(f"ARG_{argname}" for argname in argnames)
            return f"enum {{ {args} }};"
        return ""

    @staticmethod
    def fun_declare_allowed_arg(arg):
        name = arg.name
        return f"{{ MP_QSTR_{name}, MP_ARG_OBJ | MP_ARG_REQUIRED, {{0}}, }},"

    def fun_parse_args(self, args):
        body = [
            self.fun_declare_args_enum(args),
            "static const mp_arg_t allowed_args[] = {",
            *(self.fun_declare_allowed_arg(a) for a in args),
            "};",
            "mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];",
            "mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);",
        ]
        return "\n".join(f"    {line}" for line in body)

    def make_converter(self, fieldname, typedesc):
        return make_converter(self, fieldname, typedesc)

    def fun_convert_arg(self, idx, arg):
        return self.make_converter(arg.name, self.types[arg.type]).emit_to_c(
            f"args[{idx}].u_obj", arg.name or f"arg{idx}"
        )

    def fun_convert_args(self, args):
        return "".join("    " + self.fun_convert_arg(i, a) for i, a in enumerate(args))

    def fun_call_fun(self, fun):
        return_type = fun.return_
        args = fun.args
        argnames = [arg.name or f"arg{i}" for i, arg in enumerate(args)]
        fun_args = ", ".join(argnames)
        if fun.inline:
            funcall = f"{fun.inline};"
        funcall = f"{fun.name}({fun_args});"
        if return_type:
            funcall = f"{return_type} retval = {funcall}"
        return "    " + funcall

    def fun_convert_return(self, fun):
        return_type = fun.return_
        if return_type:
            converter = self.make_converter(0, self.types[return_type])
            return f"    return {converter.emit_to_py('retval')};\n"
        else:
            return "    return mp_const_none;\n"

    @emit_node.register
    def emit_function(self, node: Function):
        name = node.name
        if name in needs_glue:
            self.info.append(f"Not binding {name}, it needs glue")
            return
        args = node.args
        if node.api == 'carbon':
            return
        self.body_dedent(f"""
        static mp_obj_t {name}_fn(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {{
             {self.fun_parse_args(args)}
             {self.fun_convert_args(args)}
             {self.fun_call_fun(node)}
             {self.fun_convert_return(node)}
        }}
        MP_DEFINE_CONST_FUN_OBJ_KW({name}_obj, {len(args)}, {name}_fn);
        """)
        self.add_local(name)

    @emit_node.register
    def emit_funptr(self, node: FunPtr):
        pass  # Ignore function pointers for now

    def make_output(self, target):
        def do_print(*args):
            print(*args, file=target)

        for row in self.decls:
            do_print(row)
        for row in self.body:
            do_print(row)
            do_print()
        do_print("static const mp_rom_map_elem_t module_globals_table[] = {")
        for row in self.locals:
            do_print(f"    {row}")
        do_print("};")
        do_print("static MP_DEFINE_CONST_DICT(module_globals, module_globals_table);")
        do_print(
            textwrap.dedent(f"""
            const mp_obj_module_t {self.modname}_module = {{
                .base = {{ &mp_type_module }},
                .globals = (mp_obj_dict_t *)&module_globals,
            }};

            MP_REGISTER_MODULE(MP_QSTR_{self.modname}, {self.modname}_module);""")
        )

        for row in self.info:
            print(row, file=sys.stderr)


@click.command
@click.argument("defs_files", type=click.Path(path_type=pathlib.Path, exists=True), nargs=-1)
@click.option("-o", "--output", type=click.Path(path_type=pathlib.Path), default=None)
@click.option(
    "-t", "--typedefs", multiple=True, type=click.Path(path_type=pathlib.Path, exists=True)
)
@click.option("-m", "--modname", type=str)
@click.option("--format/-no-format", "do_format")
def main(defs_files, output, modname, typedefs, do_format=False):
    if modname is None:
        modname = defs_files[0].stem

    if output is None:
        output = pathlib.Path(f"mod{modname}.c")
    processor = Processor(modname)

    for t in typedefs:
        defs = load_defs(t)
        processor.typedefs(defs)

    defs = []
    for f in defs_files:
        defs.extend(load_defs(f))
    processor.typedefs(defs)
    processor.emit(defs)

    if do_format:
        tmpfile = output.with_suffix(".tmp.c")
        with open(tmpfile, "w") as f:
            processor.make_output(f)
        print(f"Formatting {output}: ", [script_dir / "codeformat.py", "-c", tmpfile])
        subprocess.check_call([script_dir / "codeformat.py", "-c", tmpfile])
        tmpfile.rename(output)
    else:
        with open(output, "w") as f:
            processor.make_output(f)


if __name__ == '__main__':
    main()
