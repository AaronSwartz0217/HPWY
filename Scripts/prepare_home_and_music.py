"""Editor asset setup and read-only map configuration audit; never starts PIE/gameplay."""
import json
import runpy
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
for script in ("configure_game_home.py", "configure_background_music.py"):
    runpy.run_path(str(project / "Scripts" / script), init_globals={"DEFER_EDITOR_EXIT": True})

instance_cls = unreal.load_class(None, "/Script/CrashCourse.CC_AdventureGameInstance")
instance_defaults = unreal.get_default_object(instance_cls)
adventure_cls = unreal.load_class(None, "/Script/CrashCourse.CC_AdventureGameMode")
home_cls = unreal.load_class(None, "/Script/CrashCourse.CC_MainMenuGameMode")
assert adventure_cls and home_cls
rows = []
for region in instance_defaults.get_editor_property("regions"):
    region_id = str(region.get_editor_property("id"))
    assert region_id in ("Rest", "Town", "Arena")
    # Read the World asset without activating it or saving it.
    world = region.get_editor_property("map")
    assert isinstance(world, unreal.World), region_id
    saved_mode = world.get_world_settings().get_editor_property("default_game_mode")
    rows.append({
        "id": region_id, "map": world.get_path_name(),
        "savedGameMode": saved_mode.get_path_name() if saved_mode else None,
        "runtimeGameModeFromOverride": adventure_cls.get_path_name(),
    })
home = instance_defaults.get_editor_property("main_menu_map")
assert isinstance(home, unreal.World)
assert home.get_world_settings().get_editor_property("default_game_mode") == home_cls
report = {
    "regions": rows, "homeMap": home.get_path_name(), "homeGameMode": home_cls.get_path_name(),
    "verification": "Asset existence and CDO paths; C++ OverrideGameModeClass and engine call chain reviewed. No runtime test.",
    "gameplayMapsEdited": False, "gameStarted": False, "saveFilesEdited": False,
}
output = project / "Saved/UI/TravelGameModeAudit.json"
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("HOME_MUSIC_AND_MAP_AUDIT_READY " + str(output))
unreal.SystemLibrary.quit_editor()
