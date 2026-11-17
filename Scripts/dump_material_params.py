import unreal

OUT = r"C:\HPWH\Saved\mi_params.txt"
paths = [
    "/Game/HPWH/Assets/Models/书堆/Materials/Designer_Logic_PBR",
    "/Game/HPWH/Assets/Models/八卦镜/Materials/Designer_Logic_PBR",
    "/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque",
]

lines = []
for path in paths:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    lines.append("ASSET " + path + " -> " + (asset.get_class().get_name() if asset else "None"))
    if asset is None:
        continue
    try:
        for prop in asset.get_editor_property("texture_parameter_values"):
            value = prop.parameter_value
            lines.append("  TEX {} = {}".format(prop.parameter_info.name,
                value.get_path_name() if value else "None"))
    except Exception as exc:
        lines.append("  tex read failed: {}".format(exc))
    try:
        for prop in asset.get_editor_property("scalar_parameter_values"):
            lines.append("  SCALAR {} = {}".format(prop.parameter_info.name, prop.parameter_value))
    except Exception as exc:
        lines.append("  scalar read failed: {}".format(exc))
    try:
        for prop in asset.get_editor_property("vector_parameter_values"):
            lines.append("  VECTOR {} = {}".format(prop.parameter_info.name, prop.parameter_value))
    except Exception as exc:
        lines.append("  vector read failed: {}".format(exc))

with open(OUT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
print("HPWH_MI_PARAMS written")
