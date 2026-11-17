"""把 音频/SFX 下的自制 wav 导入到 /Game/HPWH/Audio/SFX。

用法：
  UnrealEditor-Cmd.exe HPWH.uproject -run=pythonscript -script="...\\HPWH\\Scripts\\import_sfx.py" -unattended -nop4 -nosplash
"""

import os

import unreal

SOURCE_DIR = r"c:\Users\29717\Desktop\幻魄物语\音频\SFX"
DESTINATION = "/Game/HPWH/Audio/SFX"

NAMES = [
    "S_HPWH_Attack",
    "S_HPWH_SoulOut",
    "S_HPWH_EnemySpawn",
    "S_HPWH_CrawlerAppear",
    "S_HPWH_CardAppear",
    "S_HPWH_PuzzleSolve",
    "S_HPWH_Footstep",
    "S_HPWH_Win",
    "S_HPWH_TileLaptop",
    "S_HPWH_TileMirror",
    "S_HPWH_TileCourtyard",
]

tasks = []
for name in NAMES:
    filename = os.path.join(SOURCE_DIR, name + ".wav")
    if not os.path.exists(filename):
        unreal.log_error("HPWH: missing sound source " + filename)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

for task in tasks:
    imported = task.get_editor_property("imported_object_paths")
    unreal.log("HPWH: imported {} -> {}".format(
        task.get_editor_property("destination_name"), list(imported)))

unreal.log("HPWH: sfx import finished, {} files".format(len(tasks)))
unreal.SystemLibrary.quit_editor()
