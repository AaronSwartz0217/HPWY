import unreal

OUT = r"C:\HPWH\Saved\asset_material_check.txt"
lines = []

registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game/HPWH/Assets/Models", recursive=True)
for asset_data in assets:
    cls = str(asset_data.asset_class_path.asset_name)
    pkg = str(asset_data.package_name)
    if cls == "StaticMesh":
        mesh = asset_data.get_asset()
        lines.append("MESH " + pkg)
        try:
            for static_material in mesh.get_editor_property("static_materials"):
                mat = static_material.get_editor_property("material_interface")
                lines.append("  slot -> {}".format(mat.get_path_name() if mat else "None"))
        except Exception as exc:
            lines.append("  material read failed: {}".format(exc))
    elif cls in ("Material", "MaterialInstanceConstant"):
        lines.append("MAT " + pkg)
        try:
            deps = registry.get_dependencies(asset_data.package_name, unreal.AssetRegistryDependencyOptions())
            for dep in deps:
                lines.append("  dep -> {}".format(dep))
        except Exception as exc:
            lines.append("  deps failed: {}".format(exc))

with open(OUT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
print("HPWH_MAT_CHECK written")
