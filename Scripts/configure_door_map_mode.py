import unreal,json
from pathlib import Path
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert levels.load_level('/Game/LanTuRebuild/Maps/初始房间/场景优化')
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world();ws=w.get_world_settings();old=str(ws.get_editor_property('default_game_mode'))
ws.set_editor_property('default_game_mode',unreal.CC_AdventureGameMode)
assert levels.save_current_level()
p=Path(unreal.Paths.project_saved_dir())/'DoorInteractionSetup.json';r=json.loads(p.read_text(encoding='utf-8'));r['old_game_mode']=old;r['new_game_mode']='/Script/CrashCourse.CC_AdventureGameMode';p.write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
