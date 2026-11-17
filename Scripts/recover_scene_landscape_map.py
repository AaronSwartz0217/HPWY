"""Save an embedded-Nanite landscape map as a whole package, including private mesh exports."""
import unreal,json
from pathlib import Path
project=Path(__file__).resolve().parents[1];stage=Path(unreal.Paths.project_dir()).resolve()
assert stage.name=='China_townnight' and stage.parent==project/'Saved/SceneArchiveImport/Stages'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/baishe/baishe')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
target='/Game/场景资源/China_townnight/baishe/baishe'
assert unreal.EditorLoadingAndSavingUtils.save_map(world,target)
assert (stage/'Content/场景资源/China_townnight/baishe/baishe.umap').is_file()
unreal.log('LANDSCAPE_PACKAGE_SAVE_COMPLETE '+target)
