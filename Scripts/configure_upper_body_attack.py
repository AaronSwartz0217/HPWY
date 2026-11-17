"""Back up and layer all generated character attacks above the pelvis."""
from pathlib import Path
import json, shutil, unreal
project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
library = unreal.load_asset('/Game/HPWH/Locomotion/DA_NPCLocomotion')
assert library
backup = project/'Saved/Backups/UpperBodyAttackFix_20261027/Assets'
report = []
for entry in library.get_editor_property('entries'):
    cls = entry.get_editor_property('anim_class')
    if not cls: continue
    package = cls.get_path_name().split('.')[0]
    bp = unreal.load_asset(package)
    assert isinstance(bp, unreal.AnimBlueprint), package
    relative = Path('Content')/(package.removeprefix('/Game/')+'.uasset')
    destination = backup/relative
    if not destination.exists():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(project/relative, destination)
    spine = str(unreal.CC_AnimationSetupLibrary.configure_upper_body_attack(bp))
    assert spine not in ('None', ''), package
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp, False), package
    report.append({'blueprint':package, 'spine':spine, 'blendDepth':3})
(project/'Saved/UpperBodyAttackGraphUpdate.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('UPPER_BODY_ATTACK_GRAPHS_PASS '+str(len(report)))
