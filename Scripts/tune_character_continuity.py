"""Retune existing generated anim graphs without replacing states, clips or rules."""
from pathlib import Path
import json,shutil,unreal
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
library=unreal.load_asset('/Game/HPWH/Locomotion/DA_NPCLocomotion')
assert library
backup=project/'Saved/Backups/CharacterContinuityFix_20261027/Assets'
report=[]
for entry in library.get_editor_property('entries'):
    cls=entry.get_editor_property('anim_class')
    if not cls:continue
    package=cls.get_path_name().split('.')[0]
    bp=unreal.load_asset(package)
    assert isinstance(bp,unreal.AnimBlueprint),package
    relative=Path('Content')/(package.removeprefix('/Game/')+'.uasset')
    source=project/relative;destination=backup/relative
    if not destination.exists():
        destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
    count=unreal.CC_AnimationSetupLibrary.tune_locomotion_blends(bp)
    assert count>=31,(package,count)
    assert unreal.EditorAssetLibrary.save_loaded_asset(bp,False),package
    report.append({'blueprint':package,'updatedNodes':count})
out=project/'Saved/CharacterContinuityGraphUpdate.json'
out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
unreal.log('CHARACTER_CONTINUITY_GRAPHS_PASS '+str(len(report)))
