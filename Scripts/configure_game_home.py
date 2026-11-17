"""Create only the dedicated home map/art. Does not play the game or touch gameplay maps/saves."""
import hashlib
import json
from pathlib import Path
import unreal

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
root = "/Game/UI/MainMenu"
source = project / "SourceArt/UI/MainMenu/MainMenu_WallOnly_v3.png"
assert source.is_file(), str(source)
tools = unreal.AssetToolsHelpers.get_asset_tools()
texture_path = root + "/Textures/T_Home_WallOnly"
texture = unreal.load_asset(texture_path)
if not texture:
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = root + "/Textures"
    task.destination_name = "T_Home_WallOnly"
    task.automated = True
    task.replace_existing = False
    task.save = True
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(texture_path)
assert isinstance(texture, unreal.Texture2D), texture_path
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("srgb", True)
texture.set_editor_property("never_stream", True)
assert unreal.EditorAssetLibrary.save_loaded_asset(texture, False)

cls = unreal.load_class(None, "/Script/CrashCourse.CC_MenuArtData")
assert cls
data_path = root + "/DA_HomeArt"
data = unreal.load_asset(data_path)
created_data = data is None
if created_data:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    data = tools.create_asset("DA_HomeArt", root, cls, factory)
assert data
logo = unreal.load_asset("/Game/UI/MuralMenu/Textures/Menu/T_Menu_Logo")
assert isinstance(logo, unreal.Texture2D)
data.set_editor_property("background", texture)
data.set_editor_property("logo", logo)
intro_path = root + "/Textures/T_Home_StartupLogo"
intro = unreal.load_asset(intro_path)
if not intro:
    intro_source = project / "SourceArt/UI/MainMenu/StartupLogo_v1.png"
    assert intro_source.is_file()
    task = unreal.AssetImportTask()
    task.filename = str(intro_source)
    task.destination_path = root + "/Textures"
    task.destination_name = "T_Home_StartupLogo"
    task.automated = True
    task.save = True
    tools.import_asset_tasks([task])
    intro = unreal.load_asset(intro_path)
assert isinstance(intro, unreal.Texture2D)
intro.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
intro.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
intro.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
assert unreal.EditorAssetLibrary.save_loaded_asset(intro, False)
data.set_editor_property("startup_logo", intro)
# Keep the home lightweight: no skill gallery, character art or gameplay references.
data.set_editor_property("skill_gallery", None)
if data.get_editor_property("website_url") in ("", "https://github.com/AaronSwartz0217/UE5-SpiritTales-GAS"):
    data.set_editor_property("website_url", "https://aaronswartz0217.github.io/Spirit-Tales/")
assert unreal.EditorAssetLibrary.save_loaded_asset(data, False)

map_path = root + "/Maps/L_MainMenu"
created_map = False
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    assert levels.new_level(map_path)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None, "/Script/CrashCourse.CC_MainMenuGameMode")
    assert game_mode
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    assert levels.save_current_level()
    created_map = True

report = {
    "map": map_path,
    "mapCreated": created_map,
    "data": data_path,
    "background": texture_path,
    "startupLogo": intro_path,
    "startupHoldSeconds": 2,
    "startupFadeSeconds": 1,
    "source": str(source),
    "sourceSha256": hashlib.sha256(source.read_bytes()).hexdigest(),
    "buttons": ["开始游戏", "宣传网站", "退出游戏"],
    "gameplayMapsEdited": False,
    "gameStarted": False,
    "saveFilesEdited": False,
}
out = project / "Saved/UI/GameHomeSetup.json"
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("GAME_HOME_ASSETS_READY " + str(out))
if not globals().get("DEFER_EDITOR_EXIT", False):
    unreal.SystemLibrary.quit_editor()
