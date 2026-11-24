#!/usr/bin/env python3
import argparse
import json
import xml.etree.ElementTree as ET
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

CPP17_HEADERS = """algorithm any array atomic bitset cassert ccomplex cctype cerrno cfenv cfloat
charconv chrono cinttypes ciso646 climits clocale cmath codecvt complex condition_variable
csetjmp csignal cstdalign cstdarg cstdbool cstddef cstdint cstdio cstdlib cstring ctgmath
ctime cuchar cwchar cwctype deque exception execution filesystem forward_list fstream
functional future initializer_list iomanip ios iosfwd iostream istream iterator limits list
locale map memory memory_resource mutex new numeric optional ostream queue random ratio regex
scoped_allocator set shared_mutex sstream stack stdexcept streambuf string string_view
strstream system_error thread tuple type_traits typeindex typeinfo unordered_map
unordered_set utility valarray variant vector""".split()

C11_HEADERS = """assert.h complex.h ctype.h errno.h fenv.h float.h inttypes.h iso646.h
limits.h locale.h math.h setjmp.h signal.h stdalign.h stdarg.h stdbool.h stddef.h
stdint.h stdio.h stdlib.h string.h tgmath.h time.h uchar.h wchar.h wctype.h""".split()

CPP14_REMOVED = {"std::gets"}
CPP17_REMOVED = {
    "std::auto_ptr", "std::unary_function", "std::binary_function",
    "std::binder1st", "std::binder2nd", "std::bind1st", "std::bind2nd",
    "std::pointer_to_unary_function", "std::pointer_to_binary_function", "std::ptr_fun",
    "std::mem_fun_t", "std::mem_fun1_t", "std::const_mem_fun_t", "std::const_mem_fun1_t",
    "std::mem_fun", "std::mem_fun_ref_t", "std::mem_fun1_ref_t",
    "std::const_mem_fun_ref_t", "std::const_mem_fun1_ref_t", "std::mem_fun_ref",
    "std::unary_negate", "std::binary_negate", "std::not1", "std::not2",
    "std::random_shuffle", "std::unexpected", "std::unexpected_handler",
    "std::get_unexpected", "std::set_unexpected",
}
CPP20_INTRODUCED = {"std::formatter", "std::pointer_traits::to_address"}
CPP17_DOCUMENTATION_EXCLUSIONS = {
    "std::experimental", "std::function::assign", "std::unique_ptr::operator[]",
    "std::istreambuf_iterator::operator->", "std::basic_string::operator basic_string_view",
    "std::strstreambuf::setbuf", "std::atomic::fetch_add", "std::atomic::fetch_sub", "std::atomic::fetch_and",
    "std::atomic::fetch_or", "std::atomic::fetch_xor", "std::atomic::operator++",
    "std::atomic::operator--", "std::atomic::operator+=", "std::atomic::operator-=",
    "std::atomic::operator&=", "std::atomic::operator|=", "std::atomic::operator^=",
}
NAME_CORRECTIONS = {
    "std::filesystem::directory_options::overwrite_existing":
        "std::filesystem::copy_options::overwrite_existing",
    "std::filesystem::perms::add_perms": "std::filesystem::perm_options::add",
    "std::filesystem::perms::remove_perms": "std::filesystem::perm_options::remove",
    "std::filesystem::perms::resolve_symlinks": "std::filesystem::perm_options::nofollow",
    "std::thread::id::operator==": "std::operator==",
    "std::thread::id::operator!=": "std::operator!=",
    "std::thread::id::operator<": "std::operator<",
    "std::thread::id::operator<=": "std::operator<=",
    "std::thread::id::operator>": "std::operator>",
    "std::thread::id::operator>=": "std::operator>=",
    "std::thread::id::operator<<": "std::operator<<",
}
CPP_VERSION = {None: 3, "c++03": 3, "c++11": 11, "c++14": 14, "c++17": 17}
STANDARD_ERRNO_MACROS = {"EDOM", "EILSEQ", "ERANGE"}
MATH_VARIANT_BASES = """acos asin atan atan2 acosh asinh atanh cos sin tan cosh sinh tanh
exp exp2 expm1 log log10 log1p log2 logb ilogb cbrt fabs hypot pow sqrt erf erfc
lgamma tgamma ceil floor trunc round lround llround nearbyint rint lrint llrint frexp
ldexp modf scalbn scalbln fmod remainder remquo copysign nan nextafter nexttoward fdim
fmax fmin fma""".split()

@dataclass(frozen=True)
class ApiEntry:
    name: str
    kind: str


def removed(name):
    return any(name == item or name.startswith(item + "::")
               for item in CPP14_REMOVED | CPP17_REMOVED | CPP20_INTRODUCED
                        | CPP17_DOCUMENTATION_EXCLUSIONS)


def is_standard_entry(element):
    link = element.get("link", "")
    return not link.endswith("/error/errno_macros") or element.get("name") in STANDARD_ERRNO_MACROS


def normalize_operator(name):
    prefix, separator, base = name.rpartition("::")
    if base.startswith("operator++(") or base.startswith("operator--("):
        base = base[:10]
    elif not base.startswith("operator") and "(" in base:
        base = base.split("(", 1)[0]
    return prefix + separator + base


def namespace_of(name):
    return name.rsplit("::", 1)[0] if "::" in name else ""


def class_of(name):
    return name.rsplit("::", 1)[-1]


def qualified(parent, name):
    return parent + "::" + name if parent else name


def parse_cpp_index(filename):
    parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
    root = ET.parse(filename, parser=parser).getroot()
    entries = {}

    def add(entry):
        name = NAME_CORRECTIONS.get(entry.name, entry.name).strip()
        if not removed(name):
            entries[name] = ApiEntry(name, entry.kind)

    def visit(element, parent=""):
        if not is_standard_entry(element):
            return
        full_name = element.get("name", parent)
        if element.tag in ("class", "enum") and parent:
            full_name = qualified(parent, element.get("name"))
        if not full_name or removed(full_name):
            return
        since = element.get("since")
        if since not in CPP_VERSION:
            return
        kind = element.tag
        if kind == "constructor":
            add(ApiEntry(qualified(parent, class_of(parent)), kind))
        elif kind == "destructor":
            add(ApiEntry(qualified(parent, "~" + class_of(parent)), kind))
        elif kind == "overload":
            name = element.get("name")
            if name == "operator()":
                name = qualified(parent, name)
            elif not name.startswith("std::"):
                name = qualified(namespace_of(parent), name)
            add(ApiEntry(normalize_operator(name), "function"))
        elif kind == "inherits":
            pass
        elif kind == "specialization":
            add(ApiEntry(element.get("name"), "class"))
        else:
            name = full_name if not parent or kind in ("class", "enum") else qualified(parent, element.get("name"))
            add(ApiEntry(normalize_operator(name), kind))
        if kind in ("class", "enum"):
            protected = False
            for child in list(element):
                if child.tag is ET.Comment:
                    if child.text and child.text.strip() == "protected":
                        protected = True
                    continue
                if not protected:
                    visit(child, full_name)

    for element in list(root):
        if element.tag is not ET.Comment:
            visit(element)
    return entries


def parse_c_index(filename):
    root = ET.parse(filename).getroot()
    entries = {}
    for element in root:
        if not is_standard_entry(element):
            continue
        name = element.get("name")
        link = element.get("link", "")
        if (not name or name in {"gets", "_Imaginary_I", "imaginary"}
                or link.startswith("c/atomic") or link.startswith("c/thread")):
            continue
        if name == "_I":
            name = "I"
        elif name == "fgetss":
            name = "fgetws"
        entries[name] = ApiEntry(name, element.tag)
    for name in ("aligned_alloc", "timespec_get"):
        entries[name] = ApiEntry(name, "function")
    entries["TIME_UTC"] = ApiEntry("TIME_UTC", "const")
    for base in MATH_VARIANT_BASES:
        for suffix in ("f", "l"):
            name = base + suffix
            entries[name] = ApiEntry(name, "function")
    return entries


def validate_entries(entries, required, forbidden):
    for name in required:
        if name not in entries:
            raise ValueError("Missing required baseline entry: " + name)
    for name in entries:
        if any(name == item or name.startswith(item + "::") for item in forbidden):
            raise ValueError("Forbidden baseline entry: " + name)


def write_baseline(output, stem, headers, entries, release):
    lines = [entry.kind + "\t" + entry.name for entry in sorted(entries.values(), key=lambda item: item.name)]
    (output / (stem + ".tsv")).write_text("\n".join(lines) + "\n", encoding="utf-8")
    (output / (stem + "-headers.txt")).write_text("\n".join(headers) + "\n", encoding="utf-8")
    metadata = {
        "source": "cppreference-doc",
        "release": release,
        "source_url": "https://github.com/PeterFeicht/cppreference-doc/releases/tag/v" + release,
        "headers": headers,
        "entry_count": len(entries),
        "kinds": dict(sorted(Counter(entry.kind for entry in entries.values()).items())),
        "scope": "Public declaration names; overload signatures are not distinguished",
    }
    (output / (stem + ".json")).write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cppreference-doc", type=Path, required=True,
                        help="Extracted cppreference-doc directory")
    parser.add_argument("--output", type=Path, required=True,
                        help="Directory for generated TSV, header lists and metadata")
    parser.add_argument("--release", default="20250209")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    cpp_entries = parse_cpp_index(args.cppreference_doc / "index-functions-cpp.xml")
    c_entries = parse_c_index(args.cppreference_doc / "index-functions-c.xml")
    validate_entries(cpp_entries,
                     {"std::abort", "std::byte", "std::basic_string::size", "std::vector::size",
                      "std::map::value_compare::operator()"},
                     CPP14_REMOVED | CPP17_REMOVED | CPP20_INTRODUCED
                     | CPP17_DOCUMENTATION_EXCLUSIONS
                     | {"std::span", "std::jthread", "std::basic_string::starts_with",
                        "std::string::starts_with", "std::map::value_compare::comp"})
    validate_entries(c_entries,
                     {"aligned_alloc", "timespec_get", "TIME_UTC", "acosf", "acosl"},
                     {"gets", "thrd_create", "atomic_flag"})
    write_baseline(args.output, "public-api-cpp17", CPP17_HEADERS, cpp_entries, args.release)
    write_baseline(args.output, "public-api-c11", C11_HEADERS, c_entries, args.release)
    print(f"C++17: {len(cpp_entries)} entries, {len(CPP17_HEADERS)} headers")
    print(f"C11: {len(c_entries)} entries, {len(C11_HEADERS)} headers")


if __name__ == "__main__":
    main()
