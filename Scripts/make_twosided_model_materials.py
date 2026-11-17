import unreal

BASE = "/Game/HPWH/Assets/Models"
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path(BASE, recursive=True)

def make_material(folder, texture):
    leaf = folder.split("/")[-1]
    mat_name = "M_" + leaf + "_TwoSided"
    mat_path = folder + "/Materials/" + mat_name
    existing = unreal.EditorAssetLibrary.load_asset(mat_path)
    if existing:
        unreal.EditorAssetLibrary.delete_asset(mat_path)
    material = asset_tools.create_asset(mat_name, folder + "/Materials", unreal.Material,
        unreal.MaterialFactoryNew())
    material.set_editor_property("two_sided", True)
    # 实例化静态网格需要显式开启使用标记，否则会退回默认黑色材质。
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
    return material

# 每个模型目录取基础色贴图，建一个双面材质
folders = {}
for asset_data in assets:
    pkg = str(asset_data.package_name)
    if "/Textures/" in pkg and pkg.endswith("_texture_0"):
        folder = pkg.split("/Textures/")[0]
        tex = asset_data.get_asset()
        if tex:
            folders[folder] = tex

materials = {}
for folder, tex in folders.items():
    materials[folder] = make_material(folder, tex)
    print("HPWH_TWOSIDED material for " + folder)

# 静态网格全部换材质
for asset_data in assets:
    if str(asset_data.asset_class_path.asset_name) != "StaticMesh":
        continue
    pkg = str(asset_data.package_name)
    folder = pkg.split("/StaticMeshes/")[0]
    mat = materials.get(folder)
    mesh = asset_data.get_asset()
    if not mat or not mesh:
        continue
    slots = mesh.get_editor_property("static_materials")
    for slot in slots:
        slot.set_editor_property("material_interface", mat)
    mesh.set_editor_property("static_materials", slots)
    unreal.EditorAssetLibrary.save_asset(pkg)
    print("HPWH_TWOSIDED assigned " + pkg)
