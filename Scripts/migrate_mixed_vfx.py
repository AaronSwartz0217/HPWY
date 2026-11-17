"""Run in the isolated VFXMigrationStage project, never in the source project."""
import unreal
import json
import shutil
from pathlib import Path

DEST = Path(r'C:\Users\29717\Desktop\GASCrashCourse-main\GASCrashCourse-main')
SOURCE = Path(r'C:\Users\29717\Documents\Unreal Projects\geekugc\Content')
STAGE = DEST / 'Saved/VFXMigrationStage'
assert Path(unreal.Paths.project_dir()).resolve() == STAGE.resolve()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
options = unreal.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
packages = {str(a.package_name) for root in ['/Game/MixedVFX', '/Game/运河传送门'] for a in registry.get_assets_by_path(root, recursive=True)}
missing = set()
missing_owners = []
pending = list(packages)
while pending:
    package = pending.pop()
    for dep in registry.get_dependencies(package, options):
        dep = str(dep)
        if not dep.startswith('/Game/') or dep in packages:
            continue
        relative = dep[len('/Game/'):]
        source = next((SOURCE / (relative + ext) for ext in ['.uasset', '.umap'] if (SOURCE / (relative + ext)).exists()), None)
        if not source:
            missing.add(dep)
            missing_owners.append({'asset': package, 'missing': dep})
            continue
        target = STAGE / 'Content' / source.relative_to(SOURCE)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        for ext in ['.uexp', '.ubulk', '.uptnl', '.m.ubulk']:
            side = source.with_suffix(ext)
            if side.exists():
                shutil.copy2(side, target.with_suffix(ext))
        registry.scan_files_synchronous([str(target)], force_rescan=True)
        packages.add(dep)
        pending.append(dep)
(DEST / 'Saved/VFXMissingDependencies.json').write_text(json.dumps(missing_owners, ensure_ascii=False, indent=2), encoding='utf-8')
preview_repairs = {f'/Game/Characters/Heroes/Mannequin/Meshes/SKM_{name}': f'/Game/Characters/Mannequins/Meshes/SKM_{name}' for name in ['Manny', 'Quinn']}
assert not (missing - preview_repairs.keys()), 'Missing source dependencies: ' + repr(sorted(missing))

def destination(package):
    for old, new in [('/Game/MixedVFX/', '/Game/场景资源特效/MixedVFX/'), ('/Game/运河传送门/', '/Game/场景资源特效/传送门粒子/')]:
        if package.startswith(old):
            return new + package[len(old):]
    return '/Game/场景资源特效/MixedVFX_Dependencies/' + package[len('/Game/'):]

assets = []
renames = []
manifest = []
for package in sorted(packages):
    asset = unreal.load_asset(package)
    assert asset, 'Cannot load ' + package
    new = destination(package)
    folder, name = new.rsplit('/', 1)
    assets.append(asset)
    renames.append(unreal.AssetRenameData(asset, folder, name))
    manifest.append({'old': package, 'new': new, 'class': asset.get_class().get_name()})
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_tools.rename_referencing_soft_object_paths(
    [asset.get_outermost() for asset in assets],
    {unreal.SoftObjectPath(old + '.' + old.rsplit('/', 1)[1]): unreal.SoftObjectPath(new + '.' + new.rsplit('/', 1)[1]) for old, new in preview_repairs.items()})
assert asset_tools.rename_assets(renames)
for asset in assets:
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False), asset.get_path_name()
(DEST / 'Docs/MixedVFX迁移清单.json').write_text(json.dumps({'assets': manifest}, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('VFX_MIGRATION_SAVED: ' + str(len(manifest)))
