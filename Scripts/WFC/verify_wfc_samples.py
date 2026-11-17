"""Editor-world validation only. No BeginPlay, PIE, playable process or game world.

Loads our saved sample maps, verifies persisted instances, exercises generation
replacement/rollback on those maps in memory, and writes an evidence JSON.
Does not save level changes or touch the existing gameplay maps.
"""
import collections
import copy
import json
from pathlib import Path
import unreal

PROJECT = Path(unreal.Paths.project_dir()).resolve()
ROOT = '/SpiritWFC/Samples'
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
REPORT = {'scope': 'editor worlds only; no PIE/game; no modified maps saved', 'meshes': [], 'maps': [], 'seed_sweeps': []}
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT], True)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def signature(result):
    return [(str(c.tile_id), c.quarter_turns) for c in result.cells]


for asset_path in unreal.EditorAssetLibrary.list_assets(ROOT + '/Meshes', recursive=False):
    mesh = unreal.load_asset(asset_path)
    box = mesh.get_bounding_box()
    low, high = box.min, box.max
    row = {'asset': asset_path, 'min_cm': [low.x, low.y, low.z], 'max_cm': [high.x, high.y, high.z]}
    REPORT['meshes'].append(row)
    require(mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag') ==
        unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE, 'Triangle collision missing: ' + asset_path)
    if mesh.get_name() == 'SM_WFC_Floor':
        require(abs(high.x-low.x-200) < 0.1 and abs(high.y-low.y-200) < 0.1, 'FBX centimeter conversion is wrong')
        require(abs(low.z+60) < 0.1 and abs(high.z+60) < 0.1, 'Floor cell pivot was not preserved')
require(len(REPORT['meshes']) == 12, 'Expected twelve converted reference meshes')
for name in ('Stone', 'Road', 'Building', 'Stair', 'Roof'):
    mat = unreal.load_asset(ROOT+'/Materials/M_WFC_'+name)
    require(mat.get_editor_property('used_with_instanced_static_meshes'), 'Missing HISM material usage: '+name)
REPORT['instanced_material_usage'] = True

for map_name, expected in [('L_WFC_Planar', 144), ('L_WFC_ThreeLayers', 192)]:
    require(LEVELS.load_level(ROOT + '/Maps/' + map_name), 'Cannot open sample map')
    generators = [a for a in ACTORS.get_all_level_actors() if isinstance(a, unreal.WFCMapGenerator)]
    require(len(generators) == 1, 'Expected one generator')
    actor = generators[0]
    require(actor.get_generated_instance_count() == expected, 'Generated instance components did not persist')
    config = copy.copy(actor.get_editor_property('settings'))
    data = actor.get_editor_property('tile_set')
    displayed = copy.copy(actor.get_editor_property('displayed_result'))
    require(len(displayed.cells) == expected, 'Layout data did not persist')
    checked = unreal.WFCBlueprintLibrary.validate_wfc(data, config, displayed.cells)
    # UE Python converts a native bool + output arguments to Optional[tuple]:
    # success returns (Error, Components), failure returns None.
    require(checked is not None and checked[0] == '' and checked[1] == 1, 'Saved sample has invalid adjacency/connectivity: '+str(checked))
    groups = actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
    require(sum(group.get_instance_count() for group in groups) == expected, 'HISM instance count mismatch')
    require(all(group.get_editor_property('visible') for group in groups), 'Generated geometry is hidden')
    original_signature = signature(displayed)
    # Failure must preserve the old rendered layout and all its components.
    config.set_editor_property('width', 0)
    actor.set_editor_property('settings', config)
    require(not actor.generate_map(), 'Invalid grid unexpectedly generated')
    require(actor.get_generated_instance_count() == expected, 'Failure destroyed the previous map')
    require(signature(actor.get_editor_property('displayed_result')) == original_signature, 'Failure replaced the visible solution')
    # A renderer failure after staging a valid component must roll that staged
    # component back, while preserving every old visible instance.
    broken_data = unreal.WFCTileSet()
    good_tile = unreal.WFCTileDefinition()
    good_tile.set_editor_property('tile_id', 'GoodMesh')
    good_tile.set_editor_property('mesh', unreal.load_asset(ROOT+'/Meshes/SM_WFC_Floor'))
    bad_tile = unreal.WFCTileDefinition()
    bad_tile.set_editor_property('tile_id', 'MissingMesh')
    broken_data.set_editor_property('tiles', [good_tile, bad_tile])
    broken_config = unreal.WFCGenerationSettings()
    broken_config.set_editor_property('width', 2)
    broken_config.set_editor_property('depth', 1)
    broken_config.set_editor_property('require_connected_walkable_cells', False)
    pins = []
    for x, tile_id in enumerate(('GoodMesh', 'MissingMesh')):
        fixed = unreal.WFCPinnedCell()
        fixed.set_editor_property('coordinate', unreal.IntVector(x, 0, 0))
        fixed.set_editor_property('tile_id', tile_id)
        pins.append(fixed)
    broken_config.set_editor_property('pinned_cells', pins)
    actor.set_editor_property('tile_set', broken_data)
    actor.set_editor_property('settings', broken_config)
    require(not actor.generate_map(), 'A missing visual asset unexpectedly rendered')
    require(actor.get_generated_instance_count() == expected, 'Missing resource destroyed the previous map')
    require(len(actor.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)) == len(groups), 'Failed staging leaked components')
    actor.set_editor_property('tile_set', data)
    # A new seed must replace, not accumulate, components.
    config.set_editor_property('width', 8 if map_name.endswith('ThreeLayers') else 12)
    config.set_editor_property('seed', 1)
    actor.set_editor_property('settings', config)
    require(actor.generate_map(), 'Regeneration failed')
    require(actor.get_generated_instance_count() == expected, 'Regeneration duplicated instances')
    actor.clear_generated_map()
    require(actor.get_generated_instance_count() == 0, 'Clear leaked generated instances')
    require(actor.generate_map(), 'Regeneration after Clear failed')
    REPORT['maps'].append({'map': map_name, 'saved_instances': expected,
        'hism_groups': len(groups), 'persistence': True, 'socket_and_connectivity_check': True,
        'invalid_input_preserves_previous': True, 'missing_mesh_staging_rollback': True, 'replace_and_clear': True,
        'tile_counts': dict(collections.Counter(str(c.tile_id) for c in displayed.cells))})
    outcomes = []
    for seed in range(48):
        config.set_editor_property('seed', seed)
        result = unreal.WFCBlueprintLibrary.solve_wfc(data, config)
        require(result.status == unreal.WFCResultStatus.SUCCESS, f'{map_name} seed {seed}: {result.message}')
        require(result.walkable_components == 1 and len(result.cells) == expected, 'Incomplete/disconnected result')
        repeat = unreal.WFCBlueprintLibrary.solve_wfc(data, config)
        require(signature(repeat) == signature(result), 'Seed is not reproducible')
        checked = unreal.WFCBlueprintLibrary.validate_wfc(data, config, result.cells)
        require(checked is not None and checked[0] == '', str(checked))
        outcomes.append({'seed': seed, 'attempts': result.attempts, 'effective_seed': result.effective_seed,
            'ms': result.elapsed_milliseconds, 'steps': result.propagation_steps})
    REPORT['seed_sweeps'].append({'map': map_name, 'runs': outcomes})

# Same volumetric rules support one and two layers when incompatible demo pins
# are removed. Three-layer limit is also asserted through the reflected API.
data = unreal.load_asset(ROOT + '/Data/DA_WFC_Volume')
config = unreal.WFCGenerationSettings()
config.set_editor_property('mode', unreal.WFCMapMode.VOLUME)
config.set_editor_property('width', 4)
config.set_editor_property('depth', 4)
config.set_editor_property('constrain_horizontal_boundary', False)
layer_results = []
for layers in (1, 2, 3):
    config.set_editor_property('layers', layers)
    pins = []
    if layers > 1:
        pin = unreal.WFCPinnedCell()
        pin.set_editor_property('coordinate', unreal.IntVector(1, 1, 0))
        pin.set_editor_property('tile_id', 'BottomStair')
        pins.append(pin)
    config.set_editor_property('pinned_cells', pins)
    result = unreal.WFCBlueprintLibrary.solve_wfc(data, config)
    require(result.status == unreal.WFCResultStatus.SUCCESS, 'Volumetric layer variant failed: '+str(layers))
    layer_results.append({'layers': layers, 'cells': len(result.cells), 'components': result.walkable_components})
config.set_editor_property('layers', 4)
require(unreal.WFCBlueprintLibrary.solve_wfc(data, config).status == unreal.WFCResultStatus.INVALID_INPUT, 'Fourth floor allowed')
REPORT['layer_variants'] = layer_results
output = PROJECT / 'Saved/WFC/SampleVerificationReport.json'
output.write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('SPIRIT_WFC_VERIFIED '+json.dumps({'meshes': len(REPORT['meshes']), 'maps': REPORT['maps'],
    'seed_cases': sum(len(s['runs']) for s in REPORT['seed_sweeps']), 'layer_variants': layer_results}, ensure_ascii=False))
