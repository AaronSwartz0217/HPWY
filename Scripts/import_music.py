"""把降调后的三首音乐导入并替换 /Game/HPWH/Audio 下的同名 SoundWave。

用法：
  UnrealEditor-Cmd.exe HPWH.uproject -run=pythonscript -script="...\\HPWH\\Scripts\\import_music.py" -unattended -nop4 -nosplash
"""

import os

import unreal

SOURCE_DIR = r"c:\Users\29717\Desktop\幻魄物语\音频\梦境重编配_海底环境"
DESTINATION = "/Game/HPWH/Audio"

MUSIC = [
    ("Emergency1_海底环境.wav", "S_HPWH_Emergency1"),
    ("Emergency2_海底环境.wav", "S_HPWH_Emergency2"),
    ("Emergency3_海底环境.wav", "S_HPWH_Emergency3"),
]

tasks = []
for filename, name in MUSIC:
    path = os.path.join(SOURCE_DIR, filename)
    if not os.path.exists(path):
        unreal.log_error("HPWH: missing music source " + path)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
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
    unreal.log("HPWH: imported music {} -> {}".format(
        task.get_editor_property("destination_name"), list(imported)))

unreal.log("HPWH: music import finished, {} files".format(len(tasks)))
unreal.SystemLibrary.quit_editor()
