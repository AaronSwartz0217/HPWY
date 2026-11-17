"""Download only the upstream building mesh and provenance, never execute upstream code."""
import hashlib
import json
import urllib.request
import argparse
from pathlib import Path

project = Path(__file__).resolve().parents[2]
target = project / "SourceArt/ProceduralGeneration/WFC/ThirdParty/Marian42"
target.mkdir(parents=True, exist_ok=True)

def read(url):
    request = urllib.request.Request(url, headers={"User-Agent": "SpiritTales-WFC-asset-import"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read()

parser = argparse.ArgumentParser()
parser.add_argument("--revision", help="Pinned upstream commit SHA; avoids public API rate limits")
args = parser.parse_args()
existing = target / "source_manifest.json"
if existing.is_file():
    revision = json.loads(existing.read_text(encoding="utf-8"))["revision"]
elif args.revision:
    assert len(args.revision) == 40 and all(c in "0123456789abcdef" for c in args.revision)
    revision = args.revision
else:
    tree = json.loads(read("https://api.github.com/repos/marian42/wavefunctioncollapse/git/trees/master?recursive=1"))
    # Resolve the branch to a commit, not the tree SHA, before constructing raw URLs.
    revision = json.loads(read("https://api.github.com/repos/marian42/wavefunctioncollapse/commits/master"))["sha"]
    (target / "upstream_tree.json").write_text(json.dumps(tree, indent=2), encoding="utf-8")

files = []
for upstream, local in (("Assets/blocks.fbx", "blocks.fbx"), ("Assets/blocks.fbx.meta", "blocks.fbx.meta"),
                        ("LICENSE", "LICENSE"), ("README.md", "UPSTREAM_README.md")):
    url = f"https://raw.githubusercontent.com/marian42/wavefunctioncollapse/{revision}/{upstream}"
    data = read(url)
    destination = target / local
    if destination.is_file() and destination.read_bytes() != data:
        raise RuntimeError("Refusing to replace a different existing file: " + str(destination))
    destination.write_bytes(data)
    files.append({"upstream": upstream, "local": local, "url": url, "bytes": len(data),
                  "sha256": hashlib.sha256(data).hexdigest()})
manifest = {"repository": "https://github.com/marian42/wavefunctioncollapse", "revision": revision,
            "license": "MIT; preserve bundled copyright and license", "files": files,
            "authorArticle": "https://marian42.de/article/wfc/", "upstreamCodeExecuted": False}
existing.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(json.dumps({"revision": revision, "files": files}, ensure_ascii=False))
