#!/usr/bin/env python3
"""Docs CI checks:
1) Internal link/reference integrity across docs html files
2) Firmware version consistency against docs/theme.js
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


REPO_ROOT = Path(__file__).resolve().parents[1]
DOCS_ROOT = REPO_ROOT / "docs"
THEME_JS = DOCS_ROOT / "theme.js"
EXCLUDED_FOR_VERSION_CHECK = {
    DOCS_ROOT / "forks" / "index.html",
}
ALLOWED_VERSION_LITERALS = {
    DOCS_ROOT / "index.html": {"v0.4.4", "v0.4.5", "v0.4.6-beta.1"},
    DOCS_ROOT / "features" / "index.html": {"v0.4.5", "v0.4.6-beta.1"},
    DOCS_ROOT / "manual" / "index.html": {"v0.4.4", "v0.4.5", "v0.4.6-beta.1"},
}
EXCLUDED_FOR_THEME_REQUIREMENT = {
    DOCS_ROOT / "testdrive" / "sim" / "sequencer.html",
}
SKIP_LINK_CHECK_PREFIXES = (
    "http://",
    "https://",
    "mailto:",
    "tel:",
    "data:",
    "javascript:",
    "//",
)


@dataclass
class Ref:
    source_file: Path
    tag: str
    attr: str
    value: str
    line: int


class HtmlScanParser(HTMLParser):
    def __init__(self, source_file: Path):
        super().__init__(convert_charrefs=True)
        self.source_file = source_file
        self.ids: set[str] = set()
        self.refs: list[Ref] = []

    def handle_starttag(self, tag, attrs):
        attrs_dict = dict(attrs)
        if "id" in attrs_dict and attrs_dict["id"]:
            self.ids.add(attrs_dict["id"])
        if "name" in attrs_dict and attrs_dict["name"]:
            # Legacy anchor style
            self.ids.add(attrs_dict["name"])

        attr_name = None
        if tag in ("a", "link"):
            attr_name = "href"
        elif tag in ("script", "img", "iframe", "source"):
            attr_name = "src"

        if attr_name and attrs_dict.get(attr_name):
            self.refs.append(
                Ref(
                    source_file=self.source_file,
                    tag=tag,
                    attr=attr_name,
                    value=attrs_dict[attr_name].strip(),
                    line=self.getpos()[0],
                )
            )


def discover_html_files() -> list[Path]:
    return sorted(DOCS_ROOT.rglob("*.html"))


def parse_html(path: Path) -> HtmlScanParser:
    parser = HtmlScanParser(path)
    parser.feed(path.read_text(encoding="utf-8"))
    return parser


def extract_theme_version(variable_name: str) -> str:
    content = THEME_JS.read_text(encoding="utf-8")
    match = re.search(rf'{re.escape(variable_name)}\s*=\s*"([^"]+)"', content)
    if not match:
        raise RuntimeError(f"Cannot find {variable_name} in docs/theme.js")
    return match.group(1)


def resolve_local_path(source_file: Path, href_or_src: str) -> tuple[Path | None, str]:
    split = urlsplit(href_or_src)
    path_part = unquote(split.path or "")
    fragment = split.fragment

    if href_or_src.startswith("#"):
        return source_file, href_or_src[1:]

    if not path_part:
        return source_file, fragment

    # Site-root style references.
    if path_part.startswith("/"):
        root_path = path_part.lstrip("/")
        candidates = [DOCS_ROOT / root_path]
        if root_path.startswith("performer/"):
            candidates.append(DOCS_ROOT / root_path[len("performer/") :])
        for candidate in candidates:
            if candidate.exists():
                return candidate, fragment
        return candidates[0], fragment

    target = (source_file.parent / path_part).resolve()
    return target, fragment


def normalize_target_path(target: Path) -> Path:
    if target.exists() and target.is_dir():
        index_file = target / "index.html"
        if index_file.exists():
            return index_file
    return target


def check_links(html_files: list[Path], ids_by_file: dict[Path, set[str]]) -> list[str]:
    errors: list[str] = []
    for html_file in html_files:
        parser = parse_html(html_file)
        for ref in parser.refs:
            value = ref.value
            if not value or value.startswith(SKIP_LINK_CHECK_PREFIXES):
                continue

            target, fragment = resolve_local_path(html_file, value)
            if target is None:
                continue

            target = normalize_target_path(target)
            if not target.exists():
                errors.append(
                    f"{html_file.relative_to(REPO_ROOT)}:{ref.line} "
                    f"{ref.tag}[{ref.attr}] -> missing target '{value}' "
                    f"(resolved: {target.relative_to(REPO_ROOT)})"
                )
                continue

            if fragment and target.suffix.lower() == ".html":
                target_ids = ids_by_file.get(target)
                if target_ids is None:
                    target_ids = parse_html(target).ids
                    ids_by_file[target] = target_ids
                if fragment not in target_ids:
                    errors.append(
                        f"{html_file.relative_to(REPO_ROOT)}:{ref.line} "
                        f"{ref.tag}[{ref.attr}] -> missing anchor '#{fragment}' "
                        f"in {target.relative_to(REPO_ROOT)}"
                    )
    return errors


def check_version_consistency(html_files: list[Path], current_version: str) -> list[str]:
    errors: list[str] = []
    literal_version_re = re.compile(r"\bv\d+\.\d+\.\d+(?:-[A-Za-z0-9.*-]+)?")

    for html_file in html_files:
        if html_file in EXCLUDED_FOR_VERSION_CHECK:
            continue

        content = html_file.read_text(encoding="utf-8")
        allowed_versions = ALLOWED_VERSION_LITERALS.get(html_file, {current_version})
        for match in literal_version_re.finditer(content):
            literal = match.group(0)
            # Historical lineage marker like `v0.3.2-vinx.*` is allowed.
            if "-vinx" in literal:
                continue
            if literal not in allowed_versions:
                line = content.count("\n", 0, match.start()) + 1
                errors.append(
                    f"{html_file.relative_to(REPO_ROOT)}:{line} "
                    f"contains version literal '{literal}' "
                    f"(allowed here: {', '.join(sorted(allowed_versions))})"
                )

    return errors


def check_theme_inclusion(html_files: list[Path]) -> list[str]:
    errors: list[str] = []
    for html_file in html_files:
        if html_file in EXCLUDED_FOR_THEME_REQUIREMENT:
            continue
        content = html_file.read_text(encoding="utf-8")
        if "theme.js" not in content:
            errors.append(
                f"{html_file.relative_to(REPO_ROOT)} "
                "does not include theme.js (version banner/source can drift)"
            )
    return errors


def main() -> int:
    if not DOCS_ROOT.exists():
        print("ERROR: docs/ directory not found", file=sys.stderr)
        return 1

    html_files = discover_html_files()
    if not html_files:
        print("ERROR: no html files found under docs/", file=sys.stderr)
        return 1

    stable_version = extract_theme_version("vinxFirmwareVersion")
    beta_version = extract_theme_version("vinxBetaVersion")
    print(f"[docs-ci] Stable firmware version source: {stable_version}")
    print(f"[docs-ci] Beta firmware version source: {beta_version}")
    print(f"[docs-ci] HTML files scanned: {len(html_files)}")

    ids_by_file = {path: parse_html(path).ids for path in html_files}

    link_errors = check_links(html_files, ids_by_file)
    theme_errors = check_theme_inclusion(html_files)
    version_errors = check_version_consistency(html_files, stable_version)

    all_errors = link_errors + theme_errors + version_errors
    if all_errors:
        print("[docs-ci] FAILED")
        for err in all_errors:
            print(f"  - {err}")
        return 1

    print("[docs-ci] OK: links and version consistency passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
