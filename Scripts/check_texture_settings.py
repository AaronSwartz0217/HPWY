import unreal

OUT = r"C:\HPWH\Saved\texture_settings.txt"
paths = [
    "/Game/HPWH/Assets/Models/书堆/Textures/书堆_texture_0",
    "/Game/HPWH/Assets/Models/八卦镜/Textures/八卦镜_texture_0",
    "/Game/HPWH/Assets/Models/台灯/Textures/台灯_texture_0",
    "/Game/HPWH/Assets/Materials/T_HPWH_Floor",
]
lines = []
for path in paths:
    tex = unreal.EditorAssetLibrary.load_asset(path)
    if tex is None:
        lines.append(path + " -> missing")
        continue
    lines.append("{} srgb={} compression={} lod_group={}".format(
        path,
        tex.get_editor_property("srgb"),
        tex.get_editor_property("compression_settings"),
        tex.get_editor_property("lod_group")))

with open(OUT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
print("HPWH_TEX_SETTINGS written")
