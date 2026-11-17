import unreal

OUT = r"C:\HPWH\Saved\mesh_material_now.txt"
paths = [
    "/Game/HPWH/Assets/Models/书堆/StaticMeshes/book_pile",
    "/Game/HPWH/Assets/Models/台灯/StaticMeshes/desk_lamp",
    "/Game/HPWH/Assets/Models/八卦镜/StaticMeshes/bagua_mirror",
]
lines = []
for path in paths:
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if mesh is None:
        lines.append(path + " missing")
        continue
    for static_material in mesh.get_editor_property("static_materials"):
        mat = static_material.get_editor_property("material_interface")
        lines.append("{} -> {}".format(path, mat.get_path_name() if mat else "None"))
with open(OUT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
print("HPWH_MESH_MAT written")
