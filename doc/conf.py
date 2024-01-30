import sys
import subprocess
import re

def ver2no(version: str):
    pattern = re.compile(r'v(\d+)\.(\d+)\.(\d+)(?:-rc(\d+))?')
    match = pattern.match(version)
    assert match
    major, minor, patch, rc = match.groups()
    rc = int(rc) if rc else sys.maxsize
    return (int(major), int(minor), int(patch), rc)

def get_git_tags():
    resp = subprocess.run(
        ['git', 'tag', '--list'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True,
        )
    return resp.stdout.strip().split('\n')

def select_latest_version():
    tags = get_git_tags()
    return max(zip(tags, map(ver2no, tags)), key=lambda e: e[1])[0]

project = 'uniform-minios manual'
author = 'Zymelaii Ryer'
copyright = f'2024, {author}'
language = 'en'
version = select_latest_version()

extensions = []

exclude_patterns = []

templates_path = [
    '_templates',
]

html_theme = 'sphinx_rtd_theme'
html_favicon = '_static/favicon.ico'
html_theme_path = []
html_theme_options = {}
html_static_path = [
    '_static',
]
html_css_files = [
    'lang.css',
]
html_js_files = []

suppress_warnings = []
