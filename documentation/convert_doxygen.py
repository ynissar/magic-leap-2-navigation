#!/usr/bin/env python3
"""Convert Doxygen XML to compact AI-readable markdown.

Extracts only the navigation-relevant ML2 C++ SDK APIs (depth camera,
perception, head tracking, CV camera, camera) into token-efficient markdown
files suitable for LLM reference during the STTAR → ML2 port.

Usage:
    python convert_doxygen.py --xml-dir xml/ --output-dir output/
    python convert_doxygen.py --xml-dir xml/ --output-dir output/ \
        --groups group___d_cam,group___perception,group___head_tracking,group___c_v_camera,group___camera
"""

import argparse
import os
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field


# ── Configuration ──────────────────────────────────────────────────────────

DEFAULT_GROUPS = {
    "group___d_cam": "depth_camera",
    "group___perception": "perception",
    "group___head_tracking": "head_tracking",
    "group___c_v_camera": "cv_camera",
    "group___camera": "camera",
}

HL2_MAPPING = {
    "depth_camera": "Research Mode AHAT sensor → MLDepthCamera (depth frames, intrinsics, pose)",
    "perception": "SpatialCoordinateSystem → MLPerception snapshots + coordinate frame transforms",
    "head_tracking": "SpatialLocator → MLHeadTracking (head pose, tracking state)",
    "cv_camera": "(alternative to IR) → MLCVCamera (CV camera pose for marker detection)",
    "camera": "Camera intrinsics → MLCamera (general camera intrinsics, capture config)",
}


# ── Data models ────────────────────────────────────────────────────────────

@dataclass
class EnumValue:
    name: str
    initializer: str
    description: str


@dataclass
class Enum:
    name: str
    brief: str
    api_level: str
    values: list  # list[EnumValue]


@dataclass
class StructField:
    name: str
    type_str: str
    array_suffix: str
    description: str


@dataclass
class Struct:
    name: str
    brief: str
    api_level: str
    fields: list  # list[StructField]


@dataclass
class Param:
    name: str
    type_str: str
    direction: str
    description: str


@dataclass
class Function:
    name: str
    return_type: str
    brief: str
    api_level: str
    permission: str
    params: list  # list[Param]
    retvals: list  # list[tuple[str, str]]


@dataclass
class Module:
    title: str
    slug: str
    enums: list = field(default_factory=list)
    structs: list = field(default_factory=list)
    functions: list = field(default_factory=list)
    typedefs: list = field(default_factory=list)


# ── XML text extraction ───────────────────────────────────────────────────

def extract_text(element):
    """Recursively extract plain text from a Doxygen XML element.

    Handles <para>, <ref>, <simplesect>, <xrefsect>, <parameterlist>, etc.
    Strips XML markup but preserves ref text (type names).
    """
    if element is None:
        return ""

    parts = []
    if element.text:
        parts.append(element.text)

    for child in element:
        tag = child.tag
        if tag == "ref":
            # Keep referenced type/symbol names
            parts.append(child.text or "")
        elif tag == "simplesect":
            # Skip — we parse API Level / return separately
            pass
        elif tag == "xrefsect":
            # Skip — we parse permissions separately
            pass
        elif tag == "parameterlist":
            # Skip — we parse params separately
            pass
        elif tag in ("para", "emphasis", "bold", "computeroutput"):
            parts.append(extract_text(child))
        elif tag == "itemizedlist":
            for item in child.findall("listitem"):
                parts.append(extract_text(item))
        else:
            parts.append(extract_text(child))

        if child.tail:
            parts.append(child.tail)

    return " ".join(parts).strip()


def get_brief(element):
    """Get brief description text from a memberdef or compounddef."""
    brief_el = element.find("briefdescription")
    return extract_text(brief_el).strip() if brief_el is not None else ""


def get_detailed(element):
    """Get detailed description text (excluding special sections)."""
    detail_el = element.find("detaileddescription")
    return extract_text(detail_el).strip() if detail_el is not None else ""


def get_description(element):
    """Get best available description: brief, falling back to detailed."""
    brief = get_brief(element)
    if brief:
        return brief
    return get_detailed(element)


def get_api_level(element):
    """Extract API Level from <simplesect kind='par'> with title 'API Level: N'."""
    detail = element.find("detaileddescription")
    if detail is None:
        return ""
    for simplesect in detail.iter("simplesect"):
        if simplesect.get("kind") == "par":
            title_el = simplesect.find("title")
            if title_el is not None:
                title_text = (title_el.text or "") + (title_el.tail or "")
                if "API Level" in title_text:
                    # Extract the number — it's after "API Level:\n"
                    for part in title_text.split():
                        if part.isdigit():
                            return part
    return ""


def get_permission(element):
    """Extract required permission from <xrefsect> with title 'Required Permissions'."""
    detail = element.find("detaileddescription")
    if detail is None:
        return ""
    for xrefsect in detail.iter("xrefsect"):
        title_el = xrefsect.find("xreftitle")
        if title_el is not None and "Required Permissions" in (title_el.text or ""):
            desc_el = xrefsect.find("xrefdescription")
            perm = extract_text(desc_el).strip() if desc_el is not None else ""
            if perm.lower() == "none":
                return ""
            return perm
    return ""


def is_ensure32bits(name):
    """Check if an enum value is the Ensure32Bits padding sentinel."""
    return "Ensure32Bits" in name


def is_deprecated(element):
    """Check if a memberdef is marked deprecated."""
    detail = element.find("detaileddescription")
    if detail is None:
        return False
    for simplesect in detail.iter("simplesect"):
        if simplesect.get("kind") == "deprecated":
            return True
    return False


# ── Parsers ────────────────────────────────────────────────────────────────

def parse_struct(xml_path):
    """Parse a struct XML file into a Struct dataclass."""
    tree = ET.parse(xml_path)
    root = tree.getroot()
    compounddef = root.find("compounddef")
    if compounddef is None:
        return None

    name = compounddef.findtext("compoundname", "")
    brief = get_description(compounddef)
    api_level = get_api_level(compounddef)

    fields = []
    for sectiondef in compounddef.findall("sectiondef"):
        if sectiondef.get("kind") != "public-attrib":
            continue
        for memberdef in sectiondef.findall("memberdef"):
            if memberdef.get("kind") != "variable":
                continue
            if is_deprecated(memberdef):
                continue

            field_name = memberdef.findtext("name", "")
            type_el = memberdef.find("type")
            type_str = extract_text(type_el) if type_el is not None else ""
            argsstring = memberdef.findtext("argsstring", "")  # e.g. [5] for arrays
            desc = get_description(memberdef)

            fields.append(StructField(
                name=field_name,
                type_str=type_str,
                array_suffix=argsstring,
                description=desc,
            ))

    return Struct(name=name, brief=brief, api_level=api_level, fields=fields)


def parse_enum(memberdef):
    """Parse an enum memberdef element into an Enum dataclass."""
    name = memberdef.findtext("name", "")
    # Skip anonymous enums with only one value (constants)
    brief = get_description(memberdef)
    api_level = get_api_level(memberdef)

    values = []
    for ev in memberdef.findall("enumvalue"):
        ev_name = ev.findtext("name", "")
        if is_ensure32bits(ev_name):
            continue
        initializer = ev.findtext("initializer", "")
        desc = get_description(ev)
        values.append(EnumValue(name=ev_name, initializer=initializer, description=desc))

    return Enum(name=name, brief=brief, api_level=api_level, values=values)


def parse_function(memberdef):
    """Parse a function memberdef element into a Function dataclass."""
    name = memberdef.findtext("name", "")
    type_el = memberdef.find("type")
    return_type = extract_text(type_el) if type_el is not None else ""
    brief = get_brief(memberdef)
    api_level = get_api_level(memberdef)
    permission = get_permission(memberdef)

    # Parse params: type and declname from <param> elements
    param_types = {}
    for param_el in memberdef.findall("param"):
        p_type_el = param_el.find("type")
        p_type = extract_text(p_type_el) if p_type_el is not None else ""
        p_name = param_el.findtext("declname", "")
        if p_name:
            param_types[p_name] = p_type

    # Parse param descriptions and directions from <parameterlist kind="param">
    param_descs = {}
    param_dirs = {}
    detail = memberdef.find("detaileddescription")
    if detail is not None:
        for plist in detail.iter("parameterlist"):
            if plist.get("kind") != "param":
                continue
            for pitem in plist.findall("parameteritem"):
                namelist = pitem.find("parameternamelist")
                if namelist is None:
                    continue
                pname_el = namelist.find("parametername")
                if pname_el is None:
                    continue
                pname = pname_el.text or ""
                direction = pname_el.get("direction", "")
                pdesc_el = pitem.find("parameterdescription")
                pdesc = extract_text(pdesc_el) if pdesc_el is not None else ""
                param_descs[pname] = pdesc
                param_dirs[pname] = direction

    # Parse retvals from <parameterlist kind="retval">
    retvals = []
    if detail is not None:
        for plist in detail.iter("parameterlist"):
            if plist.get("kind") != "retval":
                continue
            for pitem in plist.findall("parameteritem"):
                namelist = pitem.find("parameternamelist")
                if namelist is None:
                    continue
                rv_name = namelist.findtext("parametername", "")
                rv_desc_el = pitem.find("parameterdescription")
                rv_desc = extract_text(rv_desc_el) if rv_desc_el is not None else ""
                retvals.append((rv_name, rv_desc))

    # Build ordered param list
    params = []
    for param_el in memberdef.findall("param"):
        p_name = param_el.findtext("declname", "")
        if not p_name:
            continue
        params.append(Param(
            name=p_name,
            type_str=param_types.get(p_name, ""),
            direction=param_dirs.get(p_name, ""),
            description=param_descs.get(p_name, ""),
        ))

    return Function(
        name=name,
        return_type=return_type,
        brief=brief,
        api_level=api_level,
        permission=permission,
        params=params,
        retvals=retvals,
    )


def parse_group(xml_path, xml_dir):
    """Parse a group XML file into a Module with enums, structs, and functions."""
    tree = ET.parse(xml_path)
    root = tree.getroot()
    compounddef = root.find("compounddef")
    if compounddef is None:
        return None

    title = compounddef.findtext("title", "")

    # Collect innerclass struct refs
    structs = []
    for innerclass in compounddef.findall("innerclass"):
        refid = innerclass.get("refid", "")
        struct_file = os.path.join(xml_dir, refid + ".xml")
        if os.path.isfile(struct_file):
            s = parse_struct(struct_file)
            if s is not None:
                structs.append(s)

    # Parse enums and functions from sectiondefs
    enums = []
    functions = []
    for sectiondef in compounddef.findall("sectiondef"):
        kind = sectiondef.get("kind", "")
        if kind == "enum":
            for memberdef in sectiondef.findall("memberdef"):
                if memberdef.get("kind") != "enum":
                    continue
                if is_deprecated(memberdef):
                    continue
                enum = parse_enum(memberdef)
                enums.append(enum)
        elif kind == "func":
            for memberdef in sectiondef.findall("memberdef"):
                if memberdef.get("kind") != "function":
                    continue
                if is_deprecated(memberdef):
                    continue
                func = parse_function(memberdef)
                functions.append(func)

    return Module(title=title, slug="", enums=enums, structs=structs, functions=functions)


# ── Markdown rendering ─────────────────────────────────────────────────────

def render_enum(enum):
    """Render an enum to compact markdown."""
    lines = []
    display_name = enum.name
    # Anonymous enums (e.g. @8) — use the first value's prefix or skip name
    if enum.name.startswith("@"):
        if enum.values:
            display_name = f"(constants)"
        else:
            return ""

    lines.append(f"### {display_name}")
    if enum.brief:
        lines.append(enum.brief)
    if enum.api_level:
        lines.append(f"API Level: {enum.api_level}")
    lines.append("")
    lines.append("| Value | Description |")
    lines.append("|---|---|")
    for v in enum.values:
        init = f" {v.initializer}" if v.initializer else ""
        desc = v.description or ""
        lines.append(f"| `{v.name}{init}` | {desc} |")
    lines.append("")
    return "\n".join(lines)


def render_struct(struct):
    """Render a struct to compact markdown."""
    lines = []
    lines.append(f"### {struct.name}")
    if struct.brief:
        lines.append(struct.brief)
    if struct.api_level:
        lines.append(f"API Level: {struct.api_level}")
    lines.append("")
    for f in struct.fields:
        suffix = f.array_suffix if f.array_suffix else ""
        desc = f" — {f.description}" if f.description else ""
        lines.append(f"- `{f.type_str} {f.name}{suffix}`{desc}")
    lines.append("")
    return "\n".join(lines)


def render_function(func):
    """Render a function to compact markdown."""
    lines = []
    # Signature
    param_strs = [f"{p.type_str} {p.name}" for p in func.params]
    sig = f"`{func.return_type} {func.name}({', '.join(param_strs)})`"
    lines.append(sig)
    if func.brief:
        lines.append(func.brief)
    if func.api_level:
        lines.append(f"API Level: {func.api_level}")
    if func.permission:
        lines.append(f"Permission: `{func.permission}`")
    # Params
    for p in func.params:
        direction = f" [{p.direction}]" if p.direction else ""
        desc = f": {p.description}" if p.description else ""
        lines.append(f"- {p.name}{direction}{desc}")
    # Return values
    if func.retvals:
        lines.append("Returns:")
        for rv_name, rv_desc in func.retvals:
            lines.append(f"- `{rv_name}` — {rv_desc}")
    lines.append("")
    return "\n".join(lines)


def render_module(module):
    """Render a full module to markdown."""
    lines = []
    lines.append(f"# {module.title}")
    lines.append("")

    if module.enums:
        lines.append("## Enums")
        lines.append("")
        for enum in module.enums:
            rendered = render_enum(enum)
            if rendered:
                lines.append(rendered)

    if module.structs:
        lines.append("## Structs")
        lines.append("")
        for struct in module.structs:
            lines.append(render_struct(struct))

    if module.functions:
        lines.append("## Functions")
        lines.append("")
        for func in module.functions:
            lines.append(render_function(func))

    return "\n".join(lines)


def render_index(modules):
    """Render the index file with module summaries and HL2→ML2 mapping."""
    lines = []
    lines.append("# ML2 C++ SDK — Navigation-Relevant APIs")
    lines.append("")
    lines.append("Reference documentation extracted from the Magic Leap 2 C++ SDK Doxygen XML.")
    lines.append("Only the APIs relevant to porting the STTAR framework (HL2 IR marker tracking → ML2) are included.")
    lines.append("")
    lines.append("## HL2 → ML2 API Mapping")
    lines.append("")
    lines.append("| HL2 Concept | ML2 API | File |")
    lines.append("|---|---|---|")
    for m in modules:
        mapping = HL2_MAPPING.get(m.slug, "")
        lines.append(f"| {mapping} | [{m.title}]({m.slug}.md) |")
    lines.append("")
    lines.append("## Modules")
    lines.append("")
    for m in modules:
        enum_count = len(m.enums)
        struct_count = len(m.structs)
        func_count = len(m.functions)
        lines.append(f"### [{m.title}]({m.slug}.md)")
        lines.append(f"{enum_count} enums, {struct_count} structs, {func_count} functions")
        lines.append("")

    return "\n".join(lines)


# ── Main ───────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Convert Doxygen XML to compact AI-readable markdown."
    )
    parser.add_argument(
        "--xml-dir", required=True,
        help="Path to Doxygen XML output directory"
    )
    parser.add_argument(
        "--output-dir", required=True,
        help="Path to output directory for markdown files"
    )
    parser.add_argument(
        "--groups", default=None,
        help="Comma-separated list of group XML basenames (without .xml). "
             "Defaults to the 5 navigation-relevant groups."
    )
    args = parser.parse_args()

    xml_dir = args.xml_dir
    output_dir = args.output_dir
    os.makedirs(output_dir, exist_ok=True)

    if args.groups:
        group_names = [g.strip() for g in args.groups.split(",")]
        groups = {}
        for g in group_names:
            # Auto-generate slug from group name
            slug = g.replace("group___", "").lower()
            # Use known slug mapping if available
            slug = DEFAULT_GROUPS.get(g, slug)
            groups[g] = slug
    else:
        groups = DEFAULT_GROUPS

    modules = []
    for group_id, slug in groups.items():
        xml_path = os.path.join(xml_dir, group_id + ".xml")
        if not os.path.isfile(xml_path):
            print(f"WARNING: {xml_path} not found, skipping")
            continue

        print(f"Parsing {group_id} → {slug}.md")
        module = parse_group(xml_path, xml_dir)
        if module is None:
            print(f"  WARNING: No compounddef found in {xml_path}")
            continue
        module.slug = slug

        # Write module markdown
        md_path = os.path.join(output_dir, slug + ".md")
        md_content = render_module(module)
        with open(md_path, "w") as f:
            f.write(md_content)
        print(f"  → {md_path} ({len(module.enums)} enums, {len(module.structs)} structs, {len(module.functions)} functions)")
        modules.append(module)

    # Write index
    index_path = os.path.join(output_dir, "_index.md")
    with open(index_path, "w") as f:
        f.write(render_index(modules))
    print(f"\nIndex → {index_path}")
    print(f"Done: {len(modules)} modules processed")


if __name__ == "__main__":
    main()
