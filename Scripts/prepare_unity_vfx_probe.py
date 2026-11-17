"""Read-only Unity dependency audit; stage one effect inside the UE project."""
from pathlib import Path
import re, json, shutil, hashlib, collections

PROJECT = Path(__file__).resolve().parents[1]
UNITY = Path(r'C:\Users\29717\Desktop\SHADER (2)')
SOURCE = UNITY / 'Assets/特效'
OUT = PROJECT / 'SourceArt/UnityVFXProbe'
OUT.mkdir(parents=True, exist_ok=True)
prefab = SOURCE / '第三方特效库/Vefects URP Collection/Easy Shockwaves VFX URP/VFX/Shockwaves/Particles/VFX_Shockwave_01_White_1s.prefab'
index = {}
for meta in (UNITY / 'Assets').rglob('*.meta'):
    match = re.search(r'^guid: ([a-f0-9]{32})', meta.read_text(encoding='utf-8-sig', errors='replace'), re.M)
    if match:
        index[match[1]] = Path(str(meta)[:-5])
visited, missing, queue = {}, set(), [prefab]
while queue:
    path = queue.pop()
    if path in visited or not path.is_file():
        continue
    blob = path.read_bytes()
    refs = set(re.findall(r'guid: ([a-f0-9]{32})', blob.decode('utf-8-sig', errors='replace'))) if path.suffix.lower() in {'.prefab','.mat','.asset','.shader','.shadergraph'} else set()
    name = re.sub('[^A-Za-z0-9_]', '_', path.stem) + '_' + hashlib.sha256(str(path.relative_to(UNITY)).encode()).hexdigest()[:8]
    dest = OUT / 'Dependencies' / (name + path.suffix)
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, dest)
    visited[path] = {'source': str(path), 'staged': str(dest), 'asset_name': name, 'sha256': hashlib.sha256(blob).hexdigest(), 'kind': path.suffix.lower()}
    for guid in refs:
        if guid == '0'*32:
            continue
        if guid in index:
            queue.append(index[guid])
        else:
            missing.add(guid)
counts = collections.Counter(p.suffix.lower() for p in SOURCE.rglob('*') if p.is_file() and p.suffix != '.meta')
report = {'unity_project': str(UNITY), 'source_prefab': str(prefab), 'inventory': dict(counts), 'dependencies': list(visited.values()), 'unresolved_guids': sorted(missing), 'scope': 'One shockwave proof of concept; no automatic prefab or particle conversion.'}
(OUT / 'manifest.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'inventory':dict(counts), 'dependencies':[{ 'name':v['asset_name'], 'kind':v['kind']} for v in visited.values()], 'unresolved_guids': sorted(missing)}, ensure_ascii=False, indent=2))
