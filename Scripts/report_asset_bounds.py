import unreal

OUT = r"C:\HPWH\Saved\asset_bounds.txt"
lines = []

registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game/HPWH/Assets/Models", recursive=True)
for asset_data in assets:
    cls = str(asset_data.asset_class_path.asset_name)
    if cls not in ("StaticMesh", "SkeletalMesh"):
        continue
    asset = asset_data.get_asset()
    if asset is None:
        continue
    try:
        if cls == "StaticMesh":
            box = asset.get_bounding_box()
            size = box.max - box.min
            lines.append("{} | {} | {:.1f} x {:.1f} x {:.1f} cm".format(
                asset_data.package_name, cls, size.x, size.y, size.z))
        else:
            bounds = asset.get_bounds()
            extent = bounds.box_extent
            lines.append("{} | {} | {:.1f} x {:.1f} x {:.1f} cm (extent)".format(
                asset_data.package_name, cls, extent.x * 2.0, extent.y * 2.0, extent.z * 2.0))
    except Exception as exc:
        lines.append("{} | {} | bounds failed: {}".format(asset_data.package_name, cls, exc))

with open(OUT, "w", encoding="utf-8") as handle:
    handle.write("\n".join(lines))
print("HPWH_BOUNDS written {}".format(len(lines)))
