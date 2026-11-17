import unreal, json
from pathlib import Path
M = unreal.MaterialEditingLibrary
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(synchronous_search=True)
report = []
for data in registry.get_assets_by_path('/Game/场景资源特效/流光特效', recursive=True):
    asset = data.get_asset()
    item = {'path': asset.get_path_name(), 'class': asset.get_class().get_name()}
    if isinstance(asset, unreal.Material):
        item['shading'] = str(asset.get_editor_property('shading_model'))
        item['blend'] = str(asset.get_editor_property('blend_mode'))
        nodes = set()
        def walk(node):
            if not node or node in nodes: return
            nodes.add(node)
            for parent in M.get_inputs_for_material_expression(asset, node): walk(parent)
        item['outputs'] = {}
        for name in ['MP_BASE_COLOR', 'MP_EMISSIVE_COLOR', 'MP_WORLD_POSITION_OFFSET', 'MP_OPACITY', 'MP_NORMAL']:
            node = M.get_material_property_input_node(asset, getattr(unreal.MaterialProperty, name))
            item['outputs'][name] = node.get_name() if node else None
            walk(node)
        item['nodes'] = []
        for node in nodes:
            entry = {'name': node.get_name(), 'class': node.get_class().get_name()}
            for prop in ['texture', 'parameter_name', 'default_value', 'code', 'material_function', 'constant']:
                try: entry[prop] = str(node.get_editor_property(prop))
                except Exception: pass
            entry['inputs'] = [n.get_name() if n else None for n in M.get_inputs_for_material_expression(asset, node)]
            item['nodes'].append(entry)
    report.append(item)
Path(r'C:\Users\29717\Desktop\GASCrashCourse-main\GASCrashCourse-main\Saved\FlowMaterialInspection.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
