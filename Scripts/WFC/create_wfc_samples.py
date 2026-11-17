"""UE editor-only import and WFC sample authoring. Never starts PIE/a game.

Uses only /SpiritWFC/Samples packages; existing sample maps are preserved unless
the command line explicitly includes -WFCRebuildSamples. No project-wide save.
"""
import json
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
PREPARED = PROJECT / 'SourceArt/ProceduralGeneration/WFC/Prepared'
ROOT = '/SpiritWFC/Samples'
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
REBUILD = '-WFCRebuildSamples' in unreal.SystemLibrary.get_command_line()
REPORT = {'scope': 'editor only; no gameplay/PIE', 'imported_models': [], 'maps': []}
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT], True)


def asset(name, folder, cls, factory):
    return unreal.load_asset(f'{ROOT}/{folder}/{name}') or TOOLS.create_asset(name, f'{ROOT}/{folder}', cls, factory)


def save(obj):
    assert unreal.EditorAssetLibrary.save_loaded_asset(obj, False), obj.get_path_name()


def material(name, rgb):
    result = asset(name, 'Materials', unreal.Material, unreal.MaterialFactoryNew())
    node = unreal.MaterialEditingLibrary.get_material_property_input_node(result, unreal.MaterialProperty.MP_BASE_COLOR)
    if not node:
        node = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant3Vector)
        node.set_editor_property('constant', unreal.LinearColor(*rgb, 1))
        unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
        roughness = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionConstant)
        roughness.set_editor_property('r', 0.8)
        unreal.MaterialEditingLibrary.connect_material_property(roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
        result.set_editor_property('two_sided', True)
        unreal.MaterialEditingLibrary.recompile_material(result)
    unreal.MaterialEditingLibrary.set_material_usage(result, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.MaterialEditingLibrary.recompile_material(result)
    save(result)
    return result


stone = material('M_WFC_Stone', (0.42, 0.44, 0.38))
road = material('M_WFC_Road', (0.22, 0.25, 0.21))
building = material('M_WFC_Building', (0.55, 0.40, 0.22))
stair_mat = material('M_WFC_Stair', (0.22, 0.40, 0.38))
roof_mat = material('M_WFC_Roof', (0.22, 0.25, 0.30))

# Explicit legacy factory keeps the FBX unit/pivot options reproducible.
unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
models = {}
for file in sorted(PREPARED.glob('*.fbx')):
    path = ROOT + '/Meshes/' + file.stem
    mesh = unreal.load_asset(path)
    was_existing = mesh is not None
    if not mesh:
        options = unreal.FbxImportUI()
        options.set_editor_property('automated_import_should_detect_type', False)
        options.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
        for key, value in {'import_mesh': True, 'import_as_skeletal': False,
                           'import_materials': False, 'import_textures': False, 'import_animations': False}.items():
            options.set_editor_property(key, value)
        data = options.get_editor_property('static_mesh_import_data')
        for key, value in {'combine_meshes': True, 'auto_generate_collision': False,
                           'convert_scene': True, 'convert_scene_unit': True}.items():
            data.set_editor_property(key, value)
        data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task = unreal.AssetImportTask()
        task.filename = str(file)
        task.destination_path = ROOT + '/Meshes'
        task.destination_name = file.stem
        task.automated = True
        task.replace_existing = False
        task.save = True
        task.factory = unreal.FbxFactory()
        task.options = options
        TOOLS.import_asset_tasks([task])
        mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), path
    if not was_existing or REBUILD:
        body = mesh.get_editor_property('body_setup')
        assert body, path
        body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        body.set_editor_property('double_sided_geometry', True)
        mesh.set_material(0, building)
        save(mesh)
    models[file.stem] = mesh
    REPORT['imported_models'].append(path)
assert len(models) == 12


def definition(tile_id, mesh_name='SM_WFC_Floor', weight=1, ports=('Ground',)*4,
               up='Sky', down='Ground', walkable=True, turns=(0,), mat=None,
               min_layer=0, max_layer=2, bottom=False, top=False):
    sockets = unreal.WFCSockets()
    for key, value in zip(('positive_x', 'negative_x', 'positive_y', 'negative_y'), ports):
        sockets.set_editor_property(key, value)
    sockets.set_editor_property('up', up)
    sockets.set_editor_property('down', down)
    result = unreal.WFCTileDefinition()
    values = {'tile_id': tile_id, 'weight': weight, 'sockets': sockets,
              'allowed_yaw_quarter_turns': list(turns), 'walkable': walkable,
              'mesh': models[mesh_name], 'min_layer': min_layer, 'max_layer': max_layer,
              'bottom_layer_only': bottom, 'top_layer_only': top,
              'mesh_transform': unreal.Transform(location=unreal.Vector(0, 0, 90), scale=unreal.Vector(1.5, 1.5, 1.5)),
              'materials': [mat or stone]}
    for key, value in values.items():
        result.set_editor_property(key, value)
    return result


def tileset(name, definitions, traversable):
    existing = unreal.load_asset(f'{ROOT}/Data/{name}')
    if existing and not REBUILD:
        return existing
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', unreal.WFCTileSet)
    result = asset(name, 'Data', unreal.WFCTileSet, factory)
    result.set_editor_property('cell_size', unreal.Vector(300, 300, 300))
    result.set_editor_property('tiles', definitions)
    result.set_editor_property('traversable_sockets', set(traversable))
    save(result)
    return result


flat_definitions = [
    definition('Courtyard', weight=6),
    definition('RoadStraight', weight=3, ports=('Road', 'Road', 'Ground', 'Ground'), turns=(0, 1), mat=road),
    definition('RoadCorner', weight=2, ports=('Road', 'Ground', 'Road', 'Ground'), turns=(0, 1, 2, 3), mat=road),
    definition('RoadEnd', weight=1, ports=('Road', 'Ground', 'Ground', 'Ground'), turns=(0, 1, 2, 3), mat=road),
    definition('RoadCross', weight=0.4, ports=('Road',)*4, mat=road),
    definition('BuildingWall', 'SM_WFC_Wall', weight=0.6, walkable=False, turns=(0, 1, 2, 3), mat=building),
    definition('BuildingCorner', 'SM_WFC_WallCorner', weight=0.4, walkable=False, turns=(0, 1, 2, 3), mat=building),
    definition('BuildingDoor', 'SM_WFC_Door', weight=0.25, walkable=False, turns=(0, 1, 2, 3), mat=building),
    definition('Fountain', 'SM_WFC_Fountain', weight=0.1, walkable=False, mat=building),
]
flat = tileset('DA_WFC_Planar', flat_definitions, ('Ground', 'Road'))

volume_definitions = [definition('SingleFloor', ports=('Door',)*4, bottom=True, top=True)]
for prefix, min_layer, max_layer, bottom, top, down, up in [
    ('Bottom', 0, 0, True, False, 'Ground', 'Floor'),
    ('Middle', 1, 1, False, False, 'Floor', 'Floor'),
    ('Top', 1, 2, False, True, 'Floor', 'Sky'),
]:
    for suffix, mesh_name, weight in [('Floor', 'SM_WFC_Floor', 8), ('Hall', 'SM_WFC_InteriorFloor', 1.4)]:
        volume_definitions.append(definition(prefix+suffix, mesh_name, weight=weight, ports=('Door',)*4,
            up=up, down=down, min_layer=min_layer, max_layer=max_layer, bottom=bottom, top=top))
    stair_mesh = {'Bottom': 'SM_WFC_StairsLower', 'Middle': 'SM_WFC_EnclosedStairs', 'Top': 'SM_WFC_StairsCeiling'}[prefix]
    volume_definitions.append(definition(prefix+'Stair', stair_mesh, weight=0.25, ports=('Door',)*4,
        down='Ground' if bottom else 'Stair', up='Sky' if top else 'Stair',
        min_layer=min_layer, max_layer=max_layer, bottom=bottom, top=top, mat=stair_mat))
volume_definitions.append(definition('TopRoof', 'SM_WFC_Roof', weight=0.8, ports=('Door',)*4,
    down='Floor', up='Sky', min_layer=1, top=True, walkable=False, mat=roof_mat))
volume = tileset('DA_WFC_Volume', volume_definitions, ('Door', 'Stair'))


def pin(x, y, z, tile):
    result = unreal.WFCPinnedCell()
    result.set_editor_property('coordinate', unreal.IntVector(x, y, z))
    result.set_editor_property('tile_id', tile)
    return result


def settings(volume_mode=False):
    result = unreal.WFCGenerationSettings()
    for key, value in {'mode': unreal.WFCMapMode.VOLUME if volume_mode else unreal.WFCMapMode.PLANAR,
            'width': 8 if volume_mode else 12, 'depth': 8 if volume_mode else 12,
            'layers': 3, 'seed': 20261026, 'max_restarts': 16,
            'horizontal_boundary_socket': 'Ground', 'constrain_horizontal_boundary': not volume_mode,
            'require_connected_walkable_cells': True,
            'pinned_cells': [pin(3, 3, 0, 'BottomStair')] if volume_mode else [pin(6, 6, 0, 'RoadCross'), pin(0, 0, 0, 'Courtyard')]
    }.items():
        result.set_editor_property(key, value)
    return result


def blueprint(name, data, config):
    existing = unreal.load_asset(f'{ROOT}/Blueprints/{name}')
    if existing and not REBUILD:
        return existing
    factory = unreal.BlueprintFactory()
    factory.set_editor_property('parent_class', unreal.WFCMapGenerator)
    bp = asset(name, 'Blueprints', unreal.Blueprint, factory)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.set_editor_property('tile_set', data)
    cdo.set_editor_property('settings', config)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    save(bp)
    return bp


flat_bp = blueprint('BP_WFC_Planar', flat, settings())
volume_bp = blueprint('BP_WFC_ThreeLayers', volume, settings(True))


def new_demo_map(name):
    path = ROOT + '/Maps/' + name
    exists = unreal.EditorAssetLibrary.does_asset_exist(path)
    if exists and not REBUILD:
        REPORT['maps'].append({'path': path, 'preserved_existing': True})
        return None
    assert path in (ROOT+'/Maps/L_WFC_Planar', ROOT+'/Maps/L_WFC_ThreeLayers', ROOT+'/Maps/L_WFC_ModelGallery')
    if exists:
        assert LEVELS.load_level(path), path
        owned = []
        for item in ACTORS.get_all_level_actors():
            if isinstance(item, (unreal.WorldSettings, unreal.Brush)):
                continue
            label = item.get_actor_label()
            tagged = 'SpiritWFC.Sample.v1' in [str(t) for t in item.get_editor_property('tags')]
            # Recognize the first version of these newly authored sample maps,
            # which preceded the ownership tag. Unknown user actors abort.
            legacy = label in ('WFC Sample Sun', 'WFC Generator - Select and click Generate Map')
            legacy = legacy or (isinstance(item, unreal.PlayerStart) and label == 'PlayerStart')
            legacy = legacy or (isinstance(item, unreal.StaticMeshActor) and label in models)
            assert tagged or legacy, 'Sample contains an unowned actor; duplicate it or use a fresh map: '+label
            owned.append(item)
        for item in owned:
            assert ACTORS.destroy_actor(item)
    else:
        assert LEVELS.new_level(path), path
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode', unreal.GameModeBase)
    light = ACTORS.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-45, 35, 0))
    light.set_actor_label('WFC Sample Sun')
    light.set_editor_property('tags', ['SpiritWFC.Sample.v1'])
    light.get_component_by_class(unreal.DirectionalLightComponent).set_editor_property('intensity', 4)
    light.get_component_by_class(unreal.DirectionalLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
    return path


for name, bp in [('L_WFC_Planar', flat_bp), ('L_WFC_ThreeLayers', volume_bp)]:
    path = new_demo_map(name)
    if not path:
        continue
    generator = ACTORS.spawn_actor_from_class(bp.generated_class(), unreal.Vector(0, 0, 0))
    generator.set_actor_label('WFC Generator - Select and click Generate Map')
    generator.set_editor_property('tags', ['SpiritWFC.Sample.v1'])
    assert generator.generate_map(), str(generator.get_editor_property('last_result'))
    result = generator.get_editor_property('displayed_result')
    assert generator.get_generated_instance_count() == len(result.cells)
    start = ACTORS.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120))
    start.set_actor_label('WFC Sample PlayerStart')
    start.set_editor_property('tags', ['SpiritWFC.Sample.v1'])
    assert LEVELS.save_current_level()
    REPORT['maps'].append({'path': path, 'cells': len(result.cells), 'instances': generator.get_generated_instance_count(),
        'seed': result.effective_seed, 'attempts': result.attempts, 'components': result.walkable_components,
        'milliseconds': result.elapsed_milliseconds})

path = new_demo_map('L_WFC_ModelGallery')
if path:
    for index, (name, mesh) in enumerate(models.items()):
        item = ACTORS.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector((index % 4)*450, (index//4)*450, 90))
        item.set_actor_label(name)
        item.set_editor_property('tags', ['SpiritWFC.Sample.v1'])
        component = item.get_component_by_class(unreal.StaticMeshComponent)
        component.set_static_mesh(mesh)
        item.set_actor_scale3d(unreal.Vector(1.5, 1.5, 1.5))
    assert LEVELS.save_current_level()
    REPORT['maps'].append({'path': path, 'models': len(models)})

output = PROJECT / 'Saved/WFC/SampleCreationReport.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('SPIRIT_WFC_SAMPLE_CREATION ' + json.dumps(REPORT, ensure_ascii=False))
