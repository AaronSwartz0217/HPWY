"""Install the Heaven city variants and rebuild the saved city from its existing plan."""
import copy
import datetime
import json
import shutil
from collections import Counter
from pathlib import Path

import unreal

PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ROOT = '/Game/LanTuRebuild/Maps/万象天宫'
MAP = ROOT + '/万象天宫'
SOURCE = PROJECT / 'SourceArt/ProceduralGeneration/WFC/HeavenCity'
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EDIT = unreal.MaterialEditingLibrary
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# UV0 on the city kit represents 4 meters per UV unit.
COMMON = """
float2 cell = floor(UV * 34.0);
float2 f = frac(UV * 34.0);
f = f*f*(3.0-2.0*f);
float4 values = frac(sin(float4(dot(cell, float2(127.1,311.7)),
    dot(cell + float2(1,0), float2(127.1,311.7)),
    dot(cell + float2(0,1), float2(127.1,311.7)),
    dot(cell + 1.0, float2(127.1,311.7)))) * 43758.5453);
float n = lerp(lerp(values.x,values.y,f.x),lerp(values.z,values.w,f.x),f.y);
"""
# Color and grain vary within each surface. All materials use reflected light only.
SURFACES = [
    ('M_Heaven_0', (.105, .160, .075), .93, 0.0,
     'return Tint * (0.66 + 0.54*n + 0.11*sin(UV.x*23.0)*sin(UV.y*19.0));'),
    ('M_Heaven_1', (.30, .33, .34), .81, 0.0, """
float2 p = UV * float2(6,8);
p.x += fmod(abs(floor(p.y)),2.0)*0.5;
float2 dist = 0.5 - abs(frac(p)-0.5);
float seam = min(dist.x,dist.y);
float tile = frac(sin(dot(floor(p),float2(53.31,79.23))) * 17589.15);
float edge = smoothstep(0.012,0.045+length(fwidth(p))*0.55,seam);
return Tint * lerp(0.47,0.76 + 0.21*tile + 0.22*n,edge);
"""),
    ('M_Heaven_2', (.65, .57, .44), .88, 0.0,
     'return Tint * (0.83 + 0.23*n + 0.05*sin(UV.y*18.0));'),
    ('M_Heaven_3', (.22, .084, .042), .69, 0.0, """
float p = UV.x*58 + 0.45*sin(UV.y*22.0) + 0.7*n;
float grain = cos(p*6.283) * (1.0-smoothstep(0.12,0.55,fwidth(p)));
return Tint * (0.81 + 0.28*n + 0.12*grain);
"""),
    ('M_Heaven_4', (.068, .166, .144), .54, 0.0, """
float2 p = UV * float2(16,12);
float stagger = fmod(abs(floor(p.y)),2.0)*0.5;
float flutes = 0.5 + 0.5*cos((p.x+stagger)*6.283);
flutes = lerp(flutes,0.5,smoothstep(0.15,0.62,fwidth(p.x)));
float join = smoothstep(0.025,0.09+fwidth(p.y)*0.65,frac(p.y));
return Tint * ((0.56 + 0.40*flutes + 0.19*n) * lerp(0.56,1.0,join));
"""),
    ('M_Heaven_5', (.24, .24, .20), .91, 0.0,
     'return Tint * (0.75 + 0.37*n + 0.08*sin(UV.x*52.0+UV.y*31.0));'),
    ('M_Heaven_Bronze', (.44, .23, .091), .60, .65,
     'return lerp(Tint, Tint * float3(0.35, 1.14, 0.92), smoothstep(0.59,0.90,n));'),
]

# Distinct but muted glazes and facade colors share the city material graphs.
VARIANTS = {
    'IvoryPlaster': (2, (.75, .69, .55), .86),
    'CharcoalPlaster': (2, (.29, .32, .31), .88),
    'RedLacquer': (3, (.29, .058, .031), .54),
    'DarkCedar': (3, (.115, .054, .026), .76),
    'IndigoTile': (4, (.055, .113, .23), .43),
    'JadeTile': (4, (.075, .25, .195), .38),
    'ClayTile': (4, (.30, .104, .051), .71),
    'BlackTile': (4, (.075, .085, .096), .58),
}
# House/Top use the original sockets and passage geometry.
ARCHETYPES = {
    'House_Archive': (1.05, {2:'IvoryPlaster', 3:'RedLacquer', 4:'IndigoTile'}),
    'House_Workshop': (1.10, {3:'DarkCedar', 4:'ClayTile'}),
    'House_Observatory': (.72, {2:'CharcoalPlaster', 3:'DarkCedar', 4:'BlackTile'}),
    'House_BellPavilion': (.82, {2:'IvoryPlaster', 3:'RedLacquer', 4:'JadeTile'}),
    'Top_GlazedHall': (.95, {3:'RedLacquer', 4:'JadeTile'}),
    'Top_DoubleEaves': (.78, {4:'IndigoTile'}),
}

# These retain their asset paths and WFC rules; only their geometry is changed.
STAIR_MESHES = [
    f'SM_Settlement_{role}_{i}' for role in ('Base','Middle','CompoundBase') for i in (0,1)
]
REVISED_MESHES = ['SM_Settlement_Plaza', *STAIR_MESHES]


def save(obj):
    assert LIB.save_loaded_asset(obj, False), obj.get_path_name()


def asset(name, cls, factory):
    return unreal.load_asset(f'{ROOT}/Materials/{name}') or TOOLS.create_asset(
        name, f'{ROOT}/Materials', cls, factory)


def node(mat, kind, x, y, **properties):
    expr = EDIT.create_material_expression(mat, kind, x, y)
    for key, value in properties.items():
        expr.set_editor_property(key, value)
    return expr


def make_surface(row):
    name, rgb, roughness, metallic, code = row
    mat = asset(name, unreal.Material, unreal.MaterialFactoryNew())
    EDIT.delete_all_material_expressions(mat)
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, -560, -200)
    color = node(mat, unreal.MaterialExpressionVectorParameter, -560, -65,
                 parameter_name='Tint', default_value=unreal.LinearColor(*rgb, 1))
    custom = node(mat, unreal.MaterialExpressionCustom, -235, -130, description='Heaven surface pattern',
                  output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3, code=COMMON + code.replace('Tint', 'Tint.rgb'))
    inputs = [unreal.CustomInput(), unreal.CustomInput()]
    for entry, label in zip(inputs, ('UV', 'Tint')):
        entry.set_editor_property('input_name', label)
    custom.set_editor_property('inputs', inputs)
    assert EDIT.connect_material_expressions(uv, '', custom, 'UV')
    assert EDIT.connect_material_expressions(color, '', custom, 'Tint')
    assert EDIT.connect_material_property(custom, '', unreal.MaterialProperty.MP_BASE_COLOR)
    for key, value, prop, y in (
        ('Roughness', roughness, unreal.MaterialProperty.MP_ROUGHNESS, 170),
        ('Metallic', metallic, unreal.MaterialProperty.MP_METALLIC, 330),
    ):
        param = node(mat, unreal.MaterialExpressionScalarParameter, -235, y,
                     parameter_name=key, default_value=value)
        assert EDIT.connect_material_property(param, '', prop)
    EDIT.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    EDIT.recompile_material(mat)
    return mat


def import_mesh(name, palette, variants):
    ui = unreal.FbxImportUI()
    for key, value in {
        'automated_import_should_detect_type':False, 'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH,
        'import_mesh':True, 'import_as_skeletal':False, 'import_materials':False,
        'import_textures':False, 'import_animations':False,
    }.items():
        ui.set_editor_property(key, value)
    settings = ui.get_editor_property('static_mesh_import_data')
    for key, value in {'combine_meshes':True, 'auto_generate_collision':False,
                       'convert_scene':True, 'convert_scene_unit':True}.items():
        settings.set_editor_property(key, value)
    settings.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    task = unreal.AssetImportTask()
    task.filename = str(SOURCE / (name+'.fbx'))
    task.destination_path = ROOT+'/Meshes'
    task.destination_name = name
    task.automated = True
    task.options = ui
    task.factory = unreal.FbxFactory()
    task.replace_existing = True
    task.save = False
    TOOLS.import_asset_tasks([task])
    mesh = unreal.load_asset(f'{ROOT}/Meshes/{name}')
    assert isinstance(mesh, unreal.StaticMesh), name
    slots = []
    for original in mesh.get_editor_property('static_materials'):
        slot = copy.copy(original)
        index = int(str(slot.get_editor_property('imported_material_slot_name')).rsplit('_', 1)[-1])
        slot.set_editor_property('material_interface', variants[index] if index in variants else palette[index])
        slots.append(slot)
    mesh.set_editor_property('static_materials', slots)
    actual = mesh.get_editor_property('static_materials')
    assert [slot.material_interface for slot in actual] == [slot.material_interface for slot in slots], name
    bounds = mesh.get_bounding_box()
    assert all(abs(v) <= 301 for v in (bounds.min.x,bounds.max.x,bounds.min.y,bounds.max.y)), name
    assert bounds.min.z <= 0, name
    if name != 'SM_Settlement_Plaza':
        assert bounds.max.z >= 350, name
    body = mesh.get_editor_property('body_setup')
    body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    body.set_editor_property('double_sided_geometry', True)
    if name in STAIR_MESHES:
        slope = unreal.WalkableSlopeOverride()
        slope.set_editor_property('walkable_slope_behavior', unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_INCREASE)
        slope.set_editor_property('walkable_slope_angle', 55.)
        body.set_editor_property('walkable_slope_override', slope)
    return mesh


def layout(result, tiles):
    return {(c.coordinate.x,c.coordinate.y,c.coordinate.z):
            (tuple(sorted(map(str, tiles[c.tile_index].tags))), c.quarter_turns) for c in result.cells}


def count_new(result):
    return Counter(str(c.tile_id) for c in result.cells if str(c.tile_id) in ARCHETYPES)


assert LEVELS.load_level(MAP), MAP
generators = [a for a in ACTORS.get_all_level_actors() if isinstance(a, unreal.WFCMapGenerator)]
assert len(generators) == 1
generator = generators[0]
data = generator.get_editor_property('tile_set')
assert data.get_path_name().startswith(ROOT+'/Data/DA_HeavenTiles.')
original = data.get_editor_property('tiles')
settings = generator.get_editor_property('settings')
old_result = generator.get_editor_property('displayed_result')
baseline = unreal.WFCBlueprintLibrary.solve_wfc(data, settings)
assert baseline.status == unreal.WFCResultStatus.SUCCESS, baseline.message
inputs = [f'SM_Heaven_{name}' for name in ARCHETYPES] + REVISED_MESHES
assert all((SOURCE/f'{name}.fbx').is_file() for name in inputs)

backup = PROJECT/'Saved/Backups/HeavenCity'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
existing = [ROOT+'/Data/DA_HeavenTiles'] + [ROOT+'/Materials/'+row[0] for row in SURFACES]
for package in existing + [ROOT+'/Meshes/'+name for name in REVISED_MESHES] + [MAP]:
    ext = '.umap' if package == MAP else '.uasset'
    file = PROJECT/'Content'/(package.removeprefix('/Game/')+ext)
    if file.is_file():
        dest = backup/file.relative_to(PROJECT)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(file, dest)

palette = [make_surface(row) for row in SURFACES]
finishes = {}
for name, (parent, rgb, rough) in VARIANTS.items():
    mat = asset('MI_Heaven_'+name, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    EDIT.set_material_instance_parent(mat, palette[parent])
    tint = unreal.VectorParameterValue()
    tint_info = unreal.MaterialParameterInfo()
    tint_info.set_editor_property('name', 'Tint')
    tint.set_editor_property('parameter_info', tint_info)
    tint.set_editor_property('parameter_value', unreal.LinearColor(*rgb, 1))
    mat.set_editor_property('vector_parameter_values', [tint])
    surface = unreal.ScalarParameterValue()
    roughness_info = unreal.MaterialParameterInfo()
    roughness_info.set_editor_property('name', 'Roughness')
    surface.set_editor_property('parameter_info', roughness_info)
    surface.set_editor_property('parameter_value', rough)
    mat.set_editor_property('scalar_parameter_values', [surface])
    EDIT.update_material_instance(mat)
    finishes[name] = mat

unreal.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
tiles = [t for t in original if str(t.tile_id) not in ARCHETYPES]
templates = {str(t.tile_id):t for t in tiles}
meshes = []
for name, (weight, finish) in ARCHETYPES.items():
    mesh = import_mesh(f'SM_Heaven_{name}', palette, {i:finishes[n] for i,n in finish.items()})
    meshes.append(mesh)
    tile = copy.copy(templates[name.split('_',1)[0]+'_0'])
    tile.set_editor_property('tile_id', name)
    tile.set_editor_property('mesh', mesh)
    tile.set_editor_property('materials', [])
    tile.set_editor_property('weight', weight)
    tiles.append(tile)
data.set_editor_property('tiles', tiles)

updated = unreal.WFCBlueprintLibrary.solve_wfc(data, settings)
assert updated.status == unreal.WFCResultStatus.SUCCESS, updated.message
assert layout(updated, tiles) == layout(baseline, original), 'City layout changed'
if len(old_result.cells) == len(updated.cells) and all(0 <= c.tile_index < len(original) for c in old_result.cells):
    assert layout(updated, tiles) == layout(old_result, original), 'Saved layout differs from current settings'
initial_types = count_new(updated)
assert len(initial_types) >= 3, initial_types

random_settings = copy.copy(settings)
random_settings.set_editor_property('layout_preset', unreal.WFCLayoutPreset.SETTLEMENT)
random_settings.set_editor_property('painted_cells', [])
seed_results = {}
seen = set()
for seed in range(12):
    random_settings.set_editor_property('seed', seed)
    result = unreal.WFCBlueprintLibrary.solve_wfc(data, random_settings)
    assert result.status == unreal.WFCResultStatus.SUCCESS and result.walkable_components == 1, (seed, result.message)
    types = count_new(result)
    seed_results[seed] = dict(types)
    seen.update(types)
assert seen == set(ARCHETYPES), seen
corrected_meshes = [import_mesh(name, palette, {}) for name in REVISED_MESHES]
assert generator.generate_map(), generator.get_editor_property('last_result').message
render_groups = [c for c in generator.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
                 if c.get_editor_property('static_mesh')]
assert all(c.get_material(0) for c in render_groups)
assert any(c.get_editor_property('static_mesh') in meshes for c in render_groups)
for obj in [*palette, *finishes.values(), *meshes, *corrected_meshes, data]:
    save(obj)
assert LEVELS.save_current_level()
report = {'map':MAP, 'instances':generator.get_generated_instance_count(), 'initial_types':dict(initial_types),
          'seed_results':seed_results, 'unchanged_roles_and_turns':True, 'backup':str(backup),
          'mesh_count':len(meshes), 'overlap_fixes':REVISED_MESHES, 'material_count':len(palette)+len(finishes)}
out = PROJECT/'Saved/WFC/HeavenCityUpgrade.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
unreal.log('HEAVEN_CITY_UPGRADE_SAVED')



