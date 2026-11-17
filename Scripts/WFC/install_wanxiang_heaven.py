"""Install the saved WFC layout as region four; preserve artist edits on rerun.

Editor commandlet only: never starts PIE, modifies saves or overwrites existing maps.
"""
import copy
import json
from pathlib import Path
import unreal

ROOT = '/Game/LanTuRebuild/Maps/万象天宫'
MAP = ROOT + '/万象天宫'
SOURCE = '/SpiritWFC/Settlement'
lib = unreal.EditorAssetLibrary
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT, SOURCE], True)

if not lib.does_asset_exist(MAP):
    materials = {}
    for i in range(6):
        path = SOURCE + '/Materials/M_Settlement_' + str(i)
        dest = ROOT + '/Materials/M_Heaven_' + str(i)
        material = unreal.load_asset(dest) or lib.duplicate_asset(path, dest)
        assert material, dest
        materials[path + '.M_Settlement_' + str(i)] = material
        assert lib.save_loaded_asset(material, False)

    meshes = {}
    for path in lib.list_assets(SOURCE + '/Meshes', recursive=False):
        old = unreal.load_asset(path)
        if not isinstance(old, unreal.StaticMesh):
            continue
        dest = ROOT + '/Meshes/' + old.get_name()
        mesh = unreal.load_asset(dest) or lib.duplicate_asset(path, dest)
        assert mesh, dest
        slots = []
        for old_slot in mesh.get_editor_property('static_materials'):
            slot = copy.copy(old_slot)
            mat = slot.get_editor_property('material_interface')
            if mat and mat.get_path_name() in materials:
                slot.set_editor_property('material_interface', materials[mat.get_path_name()])
            slots.append(slot)
        mesh.set_editor_property('static_materials', slots)
        assert lib.save_loaded_asset(mesh, False)
        meshes[old.get_path_name()] = mesh

    data_path = ROOT + '/Data/DA_HeavenTiles'
    data = unreal.load_asset(data_path) or lib.duplicate_asset(SOURCE + '/Data/DA_WFC_Settlement', data_path)
    tiles = []
    for original in data.get_editor_property('tiles'):
        tile = copy.copy(original)
        mesh = tile.get_editor_property('mesh')
        if mesh:
            tile.set_editor_property('mesh', meshes[mesh.get_path_name()])
        refs = []
        for mat in tile.get_editor_property('materials'):
            refs.append(materials.get(mat.get_path_name(), mat))
        tile.set_editor_property('materials', refs)
        tiles.append(tile)
    data.set_editor_property('tiles', tiles)
    assert lib.save_loaded_asset(data, False)
    assert lib.duplicate_asset(SOURCE + '/Maps/L_WFC_GridEditorExample', MAP)
    assert levels.load_level(MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    mode = unreal.load_class(None, '/Script/CrashCourse.CC_AdventureGameMode')
    assert mode
    world.get_world_settings().set_editor_property('default_game_mode', mode)
    generators = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.WFCMapGenerator)]
    assert len(generators) == 1
    gen = generators[0]
    gen.set_actor_label('万象天宫 - 网格场景生成器')
    gen.set_editor_property('tile_set', data)
    settings = gen.get_editor_property('settings')
    painted = []
    for original in settings.get_editor_property('painted_cells'):
        cell = copy.copy(original)
        mesh = cell.get_editor_property('building_mesh')
        if mesh:
            cell.set_editor_property('building_mesh', meshes[mesh.get_path_name()])
        painted.append(cell)
    settings.set_editor_property('painted_cells', painted)
    gen.set_editor_property('settings', settings)
    gen.set_editor_property('generate_on_begin_play', False)
    assert gen.generate_map()
    assert levels.save_current_level()

# Verify disk reload and default region registration, without a game session.
assert levels.load_level(MAP)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_world_settings().get_editor_property('default_game_mode').get_name() == 'CC_AdventureGameMode'
generators = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.WFCMapGenerator)]
assert len(generators) == 1 and generators[0].get_generated_instance_count() > 0
assert any(isinstance(a, unreal.PlayerStart) for a in actors.get_all_level_actors())
instance = unreal.get_default_object(unreal.load_class(None, '/Script/CrashCourse.CC_AdventureGameInstance'))
regions = instance.get_editor_property('regions')
region = next(r for r in regions if str(r.get_editor_property('id')) == 'Heaven')
assert region.get_editor_property('initially_unlocked')
assert region.get_editor_property('map').get_path_name() == MAP + '.万象天宫'
report = {'map': MAP, 'region': 'Heaven', 'default_unlocked': True,
          'game_mode': 'CC_AdventureGameMode', 'visible_instances': generators[0].get_generated_instance_count(),
          'scope': 'editor asset reload only, no PIE or game travel'}
output = Path(unreal.Paths.project_dir()) / 'Saved/WFC/WanxiangHeavenInstallation.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('WANXIANG_HEAVEN_INSTALLED ' + json.dumps(report, ensure_ascii=False))
