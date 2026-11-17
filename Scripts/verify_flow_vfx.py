"""Check migrated serialized references in a fresh project with no old paths."""
import unreal
import json
from pathlib import Path
DEST = Path(r'C:\Users\29717\Desktop\GASCrashCourse-main\GASCrashCourse-main')
manifest = json.loads((DEST / 'Docs/流光特效迁移清单.json').read_text(encoding='utf-8'))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
packages = {item['new'] for item in manifest['assets']}
missing, outside, failed, classes = [], [], [], {}
for item in manifest['assets']:
    package = item['new']
    asset = unreal.load_asset(package)
    if not asset:
        failed.append(package)
    classes[item['class']] = classes.get(item['class'], 0) + 1
    for dep in registry.get_dependencies(package, options):
        dep = str(dep)
        if dep.startswith('/Script/'):
            continue
        if dep.startswith('/Game/') and dep not in packages:
            outside.append({'asset': package, 'dependency': dep})
        if not unreal.EditorAssetLibrary.does_asset_exist(dep):
            missing.append({'asset': package, 'dependency': dep})
report = dict(packages=len(packages), classes=classes, missing=missing, outside=outside, failed=failed)
report['passed'] = not missing and not outside and not failed
(DEST / 'Saved/FlowVFXVerify.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
assert report['passed'], 'Inspect Saved/FlowVFXVerify.json'
unreal.log('VFX_MIGRATION_VERIFIED: ' + str(len(packages)))

