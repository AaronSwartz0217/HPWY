"""导入摇杆环与摇杆球贴图，并设置为 UI 贴图。"""

import unreal

DEST = "/Game/HPWH/UI/Sprites"
FILES = [
    ("T_HPWH_JoystickRing", r"C:/HPWH/SourceArt/UI/T_HPWH_JoystickRing.png"),
    ("T_HPWH_Orb", r"C:/HPWH/SourceArt/UI/T_HPWH_Orb.png"),
]

tools = unreal.AssetToolsHelpers.get_asset_tools()
tasks = []
for name, filename in FILES:
    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = DEST
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)

tools.import_asset_tasks(tasks)

for name, _ in FILES:
    asset_path = "%s/%s" % (DEST, name)
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture:
        texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        texture.set_editor_property("compression_no_alpha", False)
        texture.set_editor_property("srgb", True)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        unreal.log("HPWH: imported %s" % asset_path)
    else:
        unreal.log_error("HPWH: failed to import %s" % asset_path)
