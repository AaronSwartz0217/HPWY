"""Import the requested exhibition music; save a standalone loop. No gameplay/audio playback."""
import hashlib
import json
import shutil
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
source = Path(r"C:\Users\29717\Documents\Unreal Projects\数媒交互文化主题项目模板\Content\正式关卡\文物展示关卡\游戏配乐萌新写的多利亚练习小曲儿_音频.uasset")
assert source.is_file()
source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
staged = project / "Content/Audio/Music/ExhibitionSource" / source.name
staged.parent.mkdir(parents=True, exist_ok=True)
if not staged.exists():
    shutil.copy2(source, staged)
assert hashlib.sha256(staged.read_bytes()).hexdigest() == source_hash, "Staged music differs; preserve it rather than overwriting"
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Game/Audio/Music"], True)
destination = "/Game/Audio/Music/SW_ExhibitionTheme"
music = unreal.load_asset(destination)
created = music is None
if created:
    original = unreal.load_asset("/Game/Audio/Music/ExhibitionSource/" + source.stem)
    assert isinstance(original, unreal.SoundWave)
    music = unreal.EditorAssetLibrary.duplicate_asset(original.get_path_name(), destination)
assert isinstance(music, unreal.SoundWave)
if created or not music.get_editor_property("looping"):
    music.set_editor_property("looping", True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(music, False)
registry.scan_paths_synchronous(["/Game/Audio/Music"], True)
dependencies = registry.get_dependencies(unreal.Name(destination), unreal.AssetRegistryDependencyOptions()) or []
missing = [str(dep) for dep in dependencies if str(dep).startswith("/Game/") and not unreal.EditorAssetLibrary.does_asset_exist(str(dep))]
assert not missing, missing
report = {
    "source": str(source), "sourceSha256": source_hash, "asset": destination, "created": created,
    "looping": music.get_editor_property("looping"), "durationSeconds": music.get_editor_property("duration"),
    "dependencies": [str(item) for item in dependencies], "missingDependencies": missing,
    "sourceEdited": False, "gameStarted": False, "audioPlayed": False,
}
output = project / "Saved/UI/BackgroundMusicSetup.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("BACKGROUND_MUSIC_READY " + str(output))
if not globals().get("DEFER_EDITOR_EXIT", False):
    unreal.SystemLibrary.quit_editor()
