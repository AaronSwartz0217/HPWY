"""把主界面背景图导入到 /Game/HPWH/UI/Sprites。

用法：
  UnrealEditor-Cmd.exe HPWH.uproject -run=pythonscript -script="...\\HPWH\\Scripts\\import_menu_bg.py" -unattended -nop4 -nosplash
"""

import os

import unreal

SOURCE = r"c:\Users\29717\Desktop\幻魄物语\素材\menu_background.jpg"
DESTINATION = "/Game/HPWH/UI/Sprites"
NAME = "T_HPWH_MenuBg"

task = unreal.AssetImportTask()
task.set_editor_property("filename", SOURCE)
task.set_editor_property("destination_path", DESTINATION)
task.set_editor_property("destination_name", NAME)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

for path in task.get_editor_property("imported_object_paths"):
    unreal.log("HPWH: imported menu background -> {}".format(path))

asset_path = "{}/{}".format(DESTINATION, NAME)
texture = unreal.load_asset(asset_path)
if texture:
    try:
        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        unreal.EditorAssetLibrary.save_asset(asset_path)
        unreal.log("HPWH: menu background set to UI texture group")
    except Exception as error:  # noqa: BLE001
        unreal.log_warning("HPWH: could not adjust texture settings: {}".format(error))
else:
    unreal.log_error("HPWH: menu background asset missing after import")

unreal.SystemLibrary.quit_editor()
