#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import re
import sys
from http.client import IncompleteRead
from datetime import datetime
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
HTML_PATH = ROOT / "docs" / "forks" / "index.html"

LATEST_DATE_RE = re.compile(r'data-latest-release="([^"]+)">(Latest release: )([^<]+)</p>')
LATEST_VERSION_RE = re.compile(r'data-latest-version="([^"]+)">(Latest version: )([^<]+)</p>')
REPO_RE = re.compile(r'data-latest-(?:release|version)="([^"]+)"')


def format_date(iso_date: str) -> str:
    dt = datetime.fromisoformat(iso_date.replace("Z", "+00:00"))
    return dt.strftime("%b %d, %Y")


def fetch_json(url: str, headers: dict[str, str]) -> object:
    request = Request(url, headers=headers)
    with urlopen(request, timeout=20) as response:
        return json.loads(response.read().decode("utf-8"))


def fetch_latest_release(repo: str) -> tuple[str, str] | None:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "performer-forks-release-updater",
    }

    token = os.getenv("GITHUB_TOKEN") or os.getenv("GH_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    try:
        try:
            latest = fetch_json(
                f"https://api.github.com/repos/{repo}/releases/latest",
                headers,
            )
        except HTTPError as exc:
            if exc.code != 404:
                raise
            releases = fetch_json(
                f"https://api.github.com/repos/{repo}/releases?per_page=1",
                headers,
            )
            if not releases:
                print(f"{repo}: no GitHub releases")
                return None
            latest = releases[0]
    except (HTTPError, URLError, IncompleteRead, json.JSONDecodeError) as exc:
        print(f"{repo}: failed to fetch releases ({exc})", file=sys.stderr)
        return None
    published_at = latest.get("published_at")
    tag_name = latest.get("tag_name")

    if not published_at or not tag_name:
        print(f"{repo}: latest release missing published_at or tag_name")
        return None

    latest_date = format_date(published_at)
    print(f"{repo}: latest={latest_date}, version={tag_name}")
    return latest_date, tag_name


def main() -> int:
    html = HTML_PATH.read_text(encoding="utf-8")
    repos = sorted(set(REPO_RE.findall(html)))
    updates: dict[str, tuple[str, str]] = {}

    for repo in repos:
        data = fetch_latest_release(repo)
        if data:
            updates[repo] = data

    def replace_latest_date(match: re.Match[str]) -> str:
        repo, prefix, _old_value = match.groups()
        if repo not in updates:
            return match.group(0)
        latest_date, _latest_version = updates[repo]
        return f'data-latest-release="{repo}">{prefix}{latest_date}</p>'

    def replace_latest_version(match: re.Match[str]) -> str:
        repo, prefix, _old_value = match.groups()
        if repo not in updates:
            return match.group(0)
        _latest_date, latest_version = updates[repo]
        return f'data-latest-version="{repo}">{prefix}{latest_version}</p>'

    updated_html = LATEST_DATE_RE.sub(replace_latest_date, html)
    updated_html = LATEST_VERSION_RE.sub(replace_latest_version, updated_html)

    if updated_html == html:
        print("No latest-release changes needed.")
        return 0

    HTML_PATH.write_text(updated_html, encoding="utf-8")
    print(f"Updated {HTML_PATH}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
