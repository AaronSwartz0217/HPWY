"""Install the maze tiles alongside the existing city tiles, without resaving the map."""
from collections import deque
from pathlib import Path
import datetime
import json
import shutil

import unreal

ROOT = '/Game/LanTuRebuild/Maps/万象天宫'
PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
LIB = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
TILE_PATH = ROOT + '/Data/DA_HeavenTiles'
DATA = unreal.load_asset(TILE_PATH)
assert DATA
CELL = DATA.get_editor_property('cell_size')
CUBE = unreal.load_asset('/Engine/BasicShapes/Cube')
PAVING = unreal.load_asset(ROOT + '/Meshes/SM_Settlement_Plaza')
PARENT = unreal.load_asset(ROOT + '/Materials/M_Heaven_1')
assert CUBE and PAVING and PARENT
assert CELL.x > 0 and CELL.y > 0

styles = {
    'Wall': ((.10, .15, .18), .86),
    'Entry': ((.12, .27, .22), .80),
    'Exit': ((.64, .37, .105), .70),
}

def material(label):
    name = 'MI_Heaven_Maze' + label
    obj = unreal.load_asset(ROOT + '/Materials/' + name) or TOOLS.create_asset(
        name, ROOT+'/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    EDIT.set_material_instance_parent(obj, PARENT)
    rgb, roughness = styles[label]
    for field, cls, parameter, value in (
        ('vector_parameter_values', unreal.VectorParameterValue, 'Tint', unreal.LinearColor(*rgb,1)),
        ('scalar_parameter_values', unreal.ScalarParameterValue, 'Roughness', roughness),
    ):
        info = unreal.MaterialParameterInfo()
        info.set_editor_property('name', parameter)
        override = cls()
        override.set_editor_property('parameter_info', info)
        override.set_editor_property('parameter_value', value)
        obj.set_editor_property(field, [override])
    EDIT.update_material_instance(obj)
    return obj

finishes = {label: material(label) for label in styles}
maze = []
for label in ('Wall','Path','Entry','Exit'):
    tile = unreal.WFCTileDefinition()
    name = 'Maze' + label
    tile.set_editor_property('tile_id', name)
    tile.set_editor_property('tags', {name})
    tile.set_editor_property('walkable', label != 'Wall')
    tile.set_editor_property('max_layer', 0)
    sockets = unreal.WFCSockets()
    for side in ('positive_x','negative_x','positive_y','negative_y'):
        sockets.set_editor_property(side, 'Ground')
    tile.set_editor_property('sockets', sockets)
    tile.set_editor_property('mesh', CUBE if label == 'Wall' else PAVING)
    if label != 'Path':
        tile.set_editor_property('materials', [finishes[label]])
    if label == 'Wall':
        # Neighboring wall cubes meet at cell edges. The bottom meets the path base.
        tile.set_editor_property('mesh_transform', unreal.Transform(
            location=unreal.Vector(0,0,190), scale=unreal.Vector(CELL.x/100,CELL.y/100,4.2)))
    maze.append(tile)

old_tiles = DATA.get_editor_property('tiles')
tiles = [t for t in old_tiles if str(t.tile_id) not in {str(m.tile_id) for m in maze}]
DATA.set_editor_property('tiles', tiles+maze)

LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert LEVELS.load_level(ROOT + '/万象天宫')
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
generator = next(a for a in ACTORS.get_all_level_actors() if isinstance(a, unreal.WFCMapGenerator))
assert generator.get_editor_property('tile_set') == DATA
settings = generator.get_editor_property('settings')
settings.set_editor_property('layout_preset', unreal.WFCLayoutPreset.MAZE)
settings.set_editor_property('painted_cells', [])
width, depth = settings.width, settings.depth

def fingerprint(result):
    return tuple(str(c.tile_id) for c in result.cells)

def check(seed):
    settings.set_editor_property('seed', seed)
    result = unreal.WFCBlueprintLibrary.solve_wfc(DATA, settings)
    assert result.status == unreal.WFCResultStatus.SUCCESS and result.walkable_components == 1, (seed,result.message)
    ground = {(c.coordinate.x,c.coordinate.y):str(c.tile_id) for c in result.cells if c.coordinate.z == 0}
    path = {point for point, name in ground.items() if name != 'MazeWall'}
    entry, = [point for point in path if ground[point] == 'MazeEntry']
    goal, = [point for point in path if ground[point] == 'MazeExit']
    assert all(0 < x < width-1 and 0 < y < depth-1 for x,y in path)
    assert all((x,y) in path for x in range(1,4) for y in range(1,4))
    queue = deque([entry])
    dist = {entry:0}
    dead_ends = []
    while queue:
        x,y = queue.popleft()
        neighbors = path.intersection(((x+1,y),(x-1,y),(x,y+1),(x,y-1)))
        if len(neighbors) == 1:
            dead_ends.append((x,y))
        for point in neighbors - dist.keys():
            dist[point] = dist[x,y]+1
            queue.append(point)
    assert len(dist) == len(path)
    assert goal in dead_ends and dist[goal] == max(dist[p] for p in dead_ends)
    return result, dist[goal]

variants = set()
lengths = []
for seed in range(20):
    result, length = check(seed)
    variants.add(fingerprint(result))
    lengths.append(length)
assert len(variants) == 20
assert fingerprint(check(0)[0]) == fingerprint(check(0)[0])
assert len(generator.get_editor_property('displayed_result').cells) > 0
backup = PROJECT/'Saved/Backups/HeavenMaze'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
backup.mkdir(parents=True, exist_ok=True)
shutil.copy2(PROJECT/'Content/LanTuRebuild/Maps/万象天宫/Data/DA_HeavenTiles.uasset', backup/'DA_HeavenTiles.uasset')
for obj in [*finishes.values(), DATA]:
    assert LIB.save_loaded_asset(obj, False), obj
out = PROJECT/'Saved/WFC/HeavenMazeInstallation.json'
out.write_text(json.dumps({'size':[width,depth], 'verified_seeds':len(variants), 'solution_steps':lengths},
                          indent=2), encoding='utf-8')
unreal.log('HEAVEN_MAZE_ASSETS_SAVED')
