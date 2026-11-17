"""Replace leftover references to original duplicate assets inside the isolated night scene."""
import unreal,json
from pathlib import Path
project=Path(__file__).resolve().parents[1];stage=Path(unreal.Paths.project_dir()).resolve()
assert stage.name=='China_townnight' and stage.parent==project/'Saved/SceneArchiveImport/Stages'
work=project/'Saved/SceneArchiveImport'
checks=json.loads((work/'final_verification.json').read_text(encoding='utf-8'))
row=next(x for x in checks if x['folder']==stage.name)
refs=sorted({x['dependency']for x in row['unremappedOriginalReferences']})
prefix='/Game/场景资源/'+stage.name
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level(prefix+'/baishe/baishe')
# Stage-only package redirects resolve the original imports before object loading;
# saving the map serializes the canonical destination objects without deleting assets.
fixed=refs
assert levels.save_current_level()
(work/'China_townnight_leftover_fixes.json').write_text(json.dumps({'fixed':fixed},ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('SCENE_LEFTOVER_REFERENCES_FIXED '+str(len(fixed)))
