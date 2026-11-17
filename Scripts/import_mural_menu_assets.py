"""Import supplied mural menu art and configure native UMG/data references.

Run from UE 5.6's PythonScriptPlugin after compiling CrashCourseEditor and running
configure_initial_ink_nature.py. Only explicit assets below are edited. PNG files
are read unchanged; the generated-image backups are deliberately not imported.
The script does not quit the editor: the calling batch wrapper owns its lifetime.
"""
import hashlib
import json
from pathlib import Path
import struct
from datetime import datetime, timezone

import unreal


SOURCE_ROOT = Path(r"C:\Users\29717\Desktop\物魄奇谭2d资源")
UI_ROOT = "/Game/UI/MuralMenu"
DEMO_ROOT = "/Game/HPWH"

# Stable ASCII names are independent of Chinese display names and source paths.
MENU_FILES = [
    ("background", "T_Menu_Background"),
    ("logo", "T_Menu_Logo"),
    ("bag_normal", "T_Menu_Bag_Normal"),
    ("bag_active", "T_Menu_Bag_Active"),
    ("skills_normal", "T_Menu_Skills_Normal"),
    ("skills_active", "T_Menu_Skills_Active"),
    ("trade_normal", "T_Menu_Trade_Normal"),
    ("trade_active", "T_Menu_Trade_Active"),
    ("settings_normal", "T_Menu_Settings_Normal"),
    ("settings_active", "T_Menu_Settings_Active"),
]
PORTRAIT_SERIES = [
    ("初始墨人", "Ink", 1),
    ("青铜天卫", "Bronze", 4),
    ("古刹法相", "Ceramic", 5),
    ("田野陶灵", "Nature", 4),
]
TITLE_FILES = [
    ("初墨", "Ink"),
    ("青铜天卫", "Bronze"),
    ("古刹法相", "Ceramic"),
    ("田野陶灵", "Nature"),
]

# These are explicit visual bindings, never guessed from a form's display name.
# Indices here are the PNG suffixes. Stages are always ordered initial/mid/high.
FORM_BINDINGS = [
    ("DA_Form_InitialInk", "Ink", [1, 1, 1], ["初墨", "初墨", "初墨"]),
    ("DA_Form_Native", "Nature", [1, 3, 4], ["竹路人", "竹节人", "竹拳击手"]),
    ("DA_Form_Artifact_01", "Bronze", [1, 2, 3], ["铜机械小兵", "铜械战士", "铜械将军"]),
    ("DA_Form_Artifact_02", "Ceramic", [1, 3, 5], ["陶童子", "陶土行者", "陶土僧人"]),
]
UNBOUND_PORTRAITS = {
    "T_Portrait_Bronze_04": "铜械降神；当前玩家路线未开放此档",
    "T_Portrait_Nature_02": "陶罐头；保留草竹系列，当前玩家路线未开放此形态",
    "T_Portrait_Ceramic_02": "预留古刹法相形态；当前无对应玩家路线",
    "T_Portrait_Ceramic_04": "预留古刹法相形态；当前无对应玩家路线",
}

TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDITED = []


def require_asset(path, expected_type=None):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError("Required asset is missing: " + path)
    if expected_type is not None and not isinstance(asset, expected_type):
        raise RuntimeError("Unexpected asset type at: " + path)
    return asset


def native_class(name):
    cls = unreal.load_class(None, "/Script/CrashCourse." + name)
    if not cls:
        raise RuntimeError("Native class is unavailable; build CrashCourseEditor first: " + name)
    return cls


def save(asset):
    # Native property mutation can leave an existing package without the dirty flag.
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + asset.get_path_name())
    if asset not in EDITED:
        EDITED.append(asset)


def png_metadata(source):
    blob = source.read_bytes()
    if blob[:8] != b"\x89PNG\r\n\x1a\n" or blob[12:16] != b"IHDR":
        raise RuntimeError("Invalid PNG header: " + str(source))
    width, height, depth, colour_type = struct.unpack(">IIBB", blob[16:26])
    return {
        "width": width,
        "height": height,
        "bit_depth": depth,
        "png_colour_type": colour_type,
        "has_alpha_channel": colour_type in (4, 6),
        "bytes": len(blob),
        "sha256": hashlib.sha256(blob).hexdigest(),
    }


def source_records():
    records = []
    for source_name, target_name in MENU_FILES:
        records.append((Path("菜单素材") / (source_name + ".png"), "Menu", target_name))
    for source_series, target_series, count in PORTRAIT_SERIES:
        for suffix in range(1, count + 1):
            records.append((
                Path("角色壁画") / source_series / ("%s_%02d.png" % (source_series, suffix)),
                "Portraits", "T_Portrait_%s_%02d" % (target_series, suffix),
            ))
    for source_series, target_series in TITLE_FILES:
        records.append((Path("竖版称号") / (source_series + "_竖版.png"),
                        "Titles", "T_Title_" + target_series))
    return records


def import_textures(records):
    tasks = []
    report = []
    for relative, folder, name in records:
        source = SOURCE_ROOT / relative
        if not source.is_file():
            raise RuntimeError("Source artwork is missing: " + str(source))
        info = png_metadata(source)
        # Preserve the provided PNG pixel size and the full alpha channel.
        if folder != "Menu" or name != "T_Menu_Background":
            if not info["has_alpha_channel"]:
                raise RuntimeError("Expected RGBA PNG artwork: " + str(source))
        destination = UI_ROOT + "/Textures/" + folder
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", destination)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("replace_existing_settings", False)
        task.set_editor_property("save", True)
        tasks.append(task)
        report.append(dict(info, source=str(source), source_relative=str(relative),
                           asset=destination + "/" + name))
    TOOLS.import_asset_tasks(tasks)
    textures = {}
    for task, record in zip(tasks, report):
        imported = list(task.get_editor_property("imported_object_paths"))
        if not imported:
            raise RuntimeError("Texture import failed: " + record["source"])
        texture = require_asset(record["asset"], unreal.Texture2D)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("compression_no_alpha", False)
        texture.set_editor_property("never_stream", True)
        texture.set_editor_property("max_texture_size", 0)
        save(texture)
        record["imported_object"] = texture.get_path_name()
        record["unbound_reason"] = UNBOUND_PORTRAITS.get(texture.get_name(), "")
        textures[texture.get_name()] = texture
    return textures, report


def data_asset(name, cls):
    path = UI_ROOT + "/" + name
    asset = unreal.load_asset(path)
    if asset:
        return asset
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    asset = TOOLS.create_asset(name, UI_ROOT, cls, factory)
    if not asset:
        raise RuntimeError("Could not create data asset: " + path)
    return asset


def blueprint(name, parent, widget=False):
    path = UI_ROOT + "/" + name
    asset = unreal.load_asset(path)
    if asset:
        # Do not silently replace an unrelated blueprint or overwrite its graph.
        if asset.get_editor_property("parent_class") != parent:
            raise RuntimeError("Existing blueprint has a different parent: " + path)
        return asset
    factory = unreal.WidgetBlueprintFactory() if widget else unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    asset = TOOLS.create_asset(name, UI_ROOT,
                               unreal.WidgetBlueprint if widget else unreal.Blueprint, factory)
    if not asset:
        raise RuntimeError("Could not create blueprint: " + path)
    return asset


def build_art_data(textures):
    art = data_asset("DA_MenuArt", native_class("CC_MenuArtData"))
    art.set_editor_property("background", textures["T_Menu_Background"])
    art.set_editor_property("logo", textures["T_Menu_Logo"])
    art.set_editor_property("fallback_portrait", textures["T_Portrait_Ink_01"])
    art.set_editor_property("fallback_title", textures["T_Title_Ink"])
    navigation = []
    for name in ("Bag", "Skills", "Trade", "Settings"):
        row = unreal.CC_MenuNavigationArt()
        row.set_editor_property("normal", textures["T_Menu_" + name + "_Normal"])
        row.set_editor_property("active", textures["T_Menu_" + name + "_Active"])
        navigation.append(row)
    art.set_editor_property("navigation", navigation)
    save(art)
    return art


def bind_forms(textures):
    report = []
    for form_name, series, suffixes, display_names in FORM_BINDINGS:
        form_path = DEMO_ROOT + "/Forms/" + form_name
        form = require_asset(form_path)
        stages = list(form.get_editor_property("stages"))
        if len(stages) != len(suffixes):
            raise RuntimeError("Expected three stages in " + form_path)
        for tier, suffix in enumerate(suffixes):
            portrait = textures["T_Portrait_%s_%02d" % (series, suffix)]
            title = textures["T_Title_" + series]
            stage = stages[tier]
            stage.set_editor_property("menu_portrait", portrait)
            stage.set_editor_property("menu_title", title)
            stage.set_editor_property("menu_display_name", display_names[tier])
            # Unreal Python returns a struct copy: write it back into the array.
            stages[tier] = stage
            report.append({"form": form_path, "tier": tier, "display_name": display_names[tier],
                           "portrait": portrait.get_path_name(), "title": title.get_path_name()})
        form.set_editor_property("stages", stages)
        save(form)
    return report


def bind_menu_and_hud(art):
    menu = blueprint("WBP_MuralMenu", native_class("CC_MuralMenuWidget"), widget=True)
    unreal.BlueprintEditorLibrary.compile_blueprint(menu)
    menu_defaults = unreal.get_default_object(menu.generated_class())
    menu_defaults.set_editor_property("art_data", art)
    unreal.BlueprintEditorLibrary.compile_blueprint(menu)
    save(menu)

    controller = require_asset(DEMO_ROOT + "/Blueprints/BP_ProgressionController")
    pc_defaults = unreal.get_default_object(controller.generated_class())
    previous_menu = pc_defaults.get_editor_property("progression_menu_class")
    pc_defaults.set_editor_property("progression_menu_class", menu.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(controller)
    save(controller)

    # Use a Blueprint CDO so the choice is explicit and editable in the editor.
    # This is only the old screen debug panel. Character/world health widgets
    # and their ASC bindings are untouched.
    hud = blueprint("BP_MuralHUD", native_class("CC_ProgressionHUD"))
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    unreal.get_default_object(hud.generated_class()).set_editor_property("show_debug_panel", False)
    unreal.BlueprintEditorLibrary.compile_blueprint(hud)
    save(hud)
    game_mode = require_asset(DEMO_ROOT + "/Blueprints/BP_ProgressionGameMode")
    gm_defaults = unreal.get_default_object(game_mode.generated_class())
    previous_hud = gm_defaults.get_editor_property("hud_class")
    gm_defaults.set_editor_property("hud_class", hud.generated_class())
    unreal.BlueprintEditorLibrary.compile_blueprint(game_mode)
    save(game_mode)
    return {
        "widget": menu.get_path_name(), "art_data": art.get_path_name(),
        "controller": controller.get_path_name(), "hud": hud.get_path_name(),
        "game_mode": game_mode.get_path_name(),
        "previous_menu_class": previous_menu.get_path_name() if previous_menu else None,
        "previous_hud_class": previous_hud.get_path_name() if previous_hud else None,
        "show_debug_panel": False,
    }


def main():
    if not SOURCE_ROOT.is_dir():
        raise RuntimeError("UI artwork directory is missing: " + str(SOURCE_ROOT))
    # Fail before importing if the project DLL has not exposed the new types yet.
    for name in ("CC_MenuArtData", "CC_MuralMenuWidget", "CC_ProgressionHUD"):
        native_class(name)
    for form_name, _, _, _ in FORM_BINDINGS:
        require_asset(DEMO_ROOT + "/Forms/" + form_name)
    records = source_records()
    if len(records) != 28:
        raise RuntimeError("The supplied official art inventory should contain 28 PNG files")
    textures, texture_report = import_textures(records)
    art = build_art_data(textures)
    form_report = bind_forms(textures)
    ui_report = bind_menu_and_hud(art)
    manifest = {
        "schema_version": 1,
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "source_root": str(SOURCE_ROOT), "asset_root": UI_ROOT,
        "textures": texture_report, "form_stage_bindings": form_report,
        "menu_configuration": ui_report,
        "texture_policy": {"compression": "UserInterface2D (RGBA8)", "group": "UI",
                           "mipmaps": "NoMipmaps", "srgb": True, "alpha_preserved": True,
                           "source_pixels_resized": False},
        "ignored": ["生成原稿备份", "说明文档及提示词 JSON"],
        "edited_assets": [asset.get_path_name() for asset in EDITED],
    }
    saved_dir = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()))
    destination = saved_dir / "UI" / "MuralMenuImport.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.log("MURAL_MENU_IMPORT_OK: %d textures, %d stage art bindings; %s" %
               (len(texture_report), len(form_report), str(destination)))


if __name__ == "__main__":
    main()
