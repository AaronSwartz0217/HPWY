import unreal

result = unreal.EditorAssetLibrary.save_directory("/Game/HPWH/Assets", only_if_is_dirty=False, recursive=True)
print("HPWH_SAVE assets result={}".format(result))

registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game/HPWH/Assets", recursive=True)
saved = 0
for asset_data in assets:
    path = str(asset_data.package_name)
    if unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False):
        saved += 1
print("HPWH_SAVE explicit saved={} of {}".format(saved, len(assets)))
