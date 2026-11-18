"""导入三只猫的 GLB 模型，并生成双面材质。"""

import os
import unreal

SOURCE_DIR = r"C:\Users\29717\Desktop\幻魄物语\模型资产"
DEST = "/Game/HPWH/Assets/Models"
REPORT = r"C:\HPWH\Saved\cat_bounds.txt"

CATS = [
    ("耄耋猫.glb", "cat_maodie"),
    ("香蕉猫.glb", "cat_banana"),
    ("旋转猫.glb", "cat_spin"),
]

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
registry = unreal.AssetRegistryHelpers.get_asset_registry()

tasks = []
for filename, dest_name in CATS:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE_DIR, filename))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    tasks.append(task)

unreal.log("HPWH_CATS import start")
asset_tools.import_asset_tasks(tasks)
unreal.log("HPWH_CATS import done")


def ensure_twosided(folder, mesh):
    assets = registry.get_assets_by_path(folder, recursive=True)
    texture = None
    for asset_data in assets:
        pkg = str(asset_data.package_name)
        if "/Textures/" in pkg and pkg.endswith("_texture_0"):
            texture = asset_data.get_asset()
            break
    if texture is None:
        unreal.log_error("HPWH_CATS no base texture for " + folder)
        return None

    leaf = folder.split("/")[-1]
    mat_name = "M_" + leaf + "_TwoSided"
    mat_path = folder + "/Materials/" + mat_name
    if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
        unreal.EditorAssetLibrary.delete_asset(mat_path)
    material = asset_tools.create_asset(mat_name, folder + "/Materials", unreal.Material,
        unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSample, -400, 0)
    sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_property(
        sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -400, 220)
    roughness.set_editor_property("r", 0.85)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(mat_path)

    slots = mesh.get_editor_property("static_materials")
    for slot in slots:
        slot.set_editor_property("material_interface", material)
    mesh.set_editor_property("static_materials", slots)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log("HPWH_CATS twosided {} -> {}".format(mat_path, mesh.get_name()))
    return material


lines = []
for filename, dest_name in CATS:
    stem = os.path.splitext(filename)[0]
    folder = DEST + "/" + stem
    mesh_path = folder + "/StaticMeshes/" + dest_name
    mesh = unreal.EditorAssetLibrary.load_asset(mesh_path)
    if mesh is None:
        # glTF importer may split meshes; search the folder instead.
        assets = registry.get_assets_by_path(folder, recursive=True)
        for asset_data in assets:
            if str(asset_data.asset_class_path.asset_name) == "StaticMesh":
                mesh = asset_data.get_asset()
                break
    if mesh is None:
        lines.append("{} | MISSING".format(dest_name))
        unreal.log_error("HPWH_CATS missing mesh for " + dest_name)
        continue
    box = mesh.get_bounding_box()
    size = box.max - box.min
    lines.append("{} | {} | {:.1f} x {:.1f} x {:.1f} cm | min=({:.1f},{:.1f},{:.1f})".format(
        dest_name, mesh.get_path_name(), size.x, size.y, size.z, box.min.x, box.min.y, box.min.z))
    ensure_twosided(folder, mesh)

os.makedirs(os.path.dirname(REPORT), exist_ok=True)
with open(REPORT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
for line in lines:
    unreal.log("HPWH_CATS " + line)
unreal.log("HPWH_CATS report written")
