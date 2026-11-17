"""Install the home reading points, or update the existing book assets with -RestLoreTextOnly.

Each Markdown level-2 heading is one page, unless it contains level-3
headings. In that case it is a section, and each level-3 heading is a page.
"""
from pathlib import Path
import datetime
import json
import re
import shutil
import unreal

PROJECT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ROOT = '/Game/LanTuRebuild/Maps/初始房间'
BOOKS = ROOT + '/藏书'
MAP = ROOT + '/场景优化'
LIB = unreal.EditorAssetLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LEVELS = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ACTORS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LIB.make_directory(BOOKS)

specs = [
    ('West', '砚边旧记', (1175, 500), [
        '我是在这间厢房里醒来的。桌上那幅图，画着院门外的路。',
        '门外有个传送门。画师在图上标了几处，也许那扇门就通往其中一处。',
        '先认清图上的地方，再决定从哪儿走。',
    ]),
    ('East', '山河物魄志', (5730, 350), [
        '图边画着竹节、铜片和几枚散落的钱。',
        '旁边还写着各地会遇到什么。出门前，该先认一认。',
    ]),
    ('North', '长卷问答', (3308, 3070), [
        '夹在图里的这些纸，笔迹和卷角一样。',
        '画师把路画到了哪里？也许纸上留了答案。',
    ]),
]


def read_pages(title):
    source = (PROJECT / 'Docs' / (title + '.md')).read_text(encoding='utf-8-sig')
    pages = []

    def add_page(heading, body, section=''):
        text = body.strip()
        assert heading.strip() and text, (title, heading)
        page = unreal.CC_LorePage()
        page.set_editor_property('section', section)
        page.set_editor_property('title', re.sub(r'^[一二三四五六七八九十]+　', '', heading.strip()))
        page.set_editor_property('body', re.sub(r'\*\*(.+?)\*\*', r'\1', text))
        pages.append(page)

    chapters = re.split(r'^## (.+)$', source, flags=re.MULTILINE)
    for heading, body in zip(chapters[1::2], chapters[2::2]):
        entries = re.split(r'^### (.+)$', body, flags=re.MULTILINE)
        if len(entries) == 1:
            add_page(heading, body)
        else:
            assert not entries[0].strip(), (title, heading, 'Put section text under a page heading')
            for subheading, page_body in zip(entries[1::2], entries[2::2]):
                add_page(subheading, page_body, heading.strip())
    assert pages, title
    return pages


def update_book(room, title, thoughts, pages, existing_only=False):
    name = 'DA_Lore_' + room
    path = BOOKS + '/' + name
    asset = LIB.load_asset(path) if LIB.does_asset_exist(path) else None
    if not asset and not existing_only:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class', unreal.CC_LoreBook)
        asset = TOOLS.create_asset(name, BOOKS, unreal.CC_LoreBook, factory)
    assert isinstance(asset, unreal.CC_LoreBook), path
    asset.set_editor_property('title', title)
    asset.set_editor_property('pages', pages)
    asset.set_editor_property('opening_thoughts', thoughts)
    assert LIB.save_loaded_asset(asset, False)
    return asset


def page_content(pages):
    return [tuple(str(page.get_editor_property(field)) for field in ('section', 'title', 'body'))
            for page in pages]


def update_existing_books(verify_only):
    prepared = [(room, title, thoughts, read_pages(title)) for room, title, _, thoughts in specs]
    if not verify_only:
        backup = PROJECT/'Saved/Backups/RestLoreBooks'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
        backup.mkdir(parents=True, exist_ok=True)
        for room, _, _, _ in prepared:
            name = f'DA_Lore_{room}.uasset'
            shutil.copy2(PROJECT/'Content/LanTuRebuild/Maps/初始房间/藏书'/name, backup/name)
    report = {}
    for room, title, thoughts, pages in prepared:
        if verify_only:
            asset = LIB.load_asset(BOOKS + '/DA_Lore_' + room)
        else:
            asset = update_book(room, title, thoughts, pages, existing_only=True)
        actual = page_content(asset.get_editor_property('pages'))
        assert actual == page_content(pages), title
        assert str(asset.get_editor_property('title')) == title
        assert list(map(str, asset.get_editor_property('opening_thoughts'))) == thoughts
        report[room] = {'book': title, 'pages': [page[1] for page in actual], 'thoughts': len(thoughts)}
    (PROJECT/'Saved/RestLoreBookContents.json').write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    unreal.log('REST_LORE_BOOK_TEXT_VERIFIED' if verify_only else 'REST_LORE_BOOK_TEXT_SAVED')


def install_reading_points():
    assert LEVELS.load_level(MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    before = ACTORS.get_all_level_actors()
    manager = next(a for a in before if isinstance(a, unreal.CC_DoorVisibilityManager))
    zones = {str(z.get_editor_property('room_id')): z for z in manager.get_editor_property('rooms')}

    # Find a capsule-sized clear floor position within the room, beyond door interaction range.
    def pick_position(zone, desired):
        door = zone.get_editor_property('door').get_actor_location()
        center, extent = zone.get_editor_property('center'), zone.get_editor_property('half_extent')
        trace = unreal.TraceTypeQuery.TRACE_TYPE_QUERY1
        offsets = sorted(((dx, dy) for dx in range(-200, 201, 50) for dy in range(-200, 201, 50)),
                         key=lambda p: p[0]**2 + p[1]**2)
        floor_hits = []
        for dx, dy in offsets:
            x, y = desired[0]+dx, desired[1]+dy
            if abs(x-center.x) > extent.x-90 or abs(y-center.y) > extent.y-90:
                continue
            if (x-door.x)**2 + (y-door.y)**2 < 345**2:
                continue
            floor = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x,y,65), unreal.Vector(x,y,-195),
                trace, True, [], unreal.DrawDebugTrace.NONE, False)
            if floor:
                floor_hits.append((round(floor.impact_point.z, 1), round(floor.impact_normal.z, 2)))
            if not floor or not door.z - 65 < floor.impact_point.z < door.z + 40 or floor.impact_normal.z < .85:
                continue
            z = floor.impact_point.z + 101
            blocked = unreal.SystemLibrary.capsule_trace_single(world, unreal.Vector(x,y,z+3), unreal.Vector(x,y,z),
                42, 95, trace, True, [], unreal.DrawDebugTrace.NONE, False)
            if not blocked:
                return unreal.Vector(x,y,z)
        if not floor_hits:
            # The headless editor may not register query bodies for the imported interior.
            # Use the authored doorway floor height and validate the stand in a game world.
            return unreal.Vector(desired[0], desired[1], door.z + 103)
        raise RuntimeError(f'No clear book location near {desired}; floor hits {sorted(set(floor_hits))}')


    blueprint_path = BOOKS + '/BP_LoreStand'
    bp = LIB.load_asset(blueprint_path) if LIB.does_asset_exist(blueprint_path) else None
    if not bp:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class', unreal.CC_LoreStand)
        bp = TOOLS.create_asset('BP_LoreStand', BOOKS, unreal.Blueprint, factory)
    assert bp
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assert LIB.save_loaded_asset(bp, False)

    backup = PROJECT/'Saved/Backups/RestLore'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    backup.mkdir(parents=True, exist_ok=True)
    shutil.copy2(PROJECT/'Content/LanTuRebuild/Maps/初始房间/场景优化.umap', backup/'场景优化.umap')

    report = []
    for room, title, desired, thoughts in specs:
        pages = read_pages(title)
        asset = update_book(room, title, thoughts, pages)
        pos = pick_position(zones[room], desired)
        tag = 'CC.RestLore.' + room
        placed = [a for a in before if isinstance(a, unreal.CC_LoreStand) and tag in map(str, a.tags)]
        assert len(placed) <= 1
        unreal.log(f'REST_LORE_PLACING {room} {pos}')
        stand = placed[0] if placed else ACTORS.spawn_actor_from_object(bp, pos)
        assert stand, room
        unreal.log(f'REST_LORE_PLACED {room}')
        stand.set_actor_location(pos, False, True)
        stand.set_actor_label('藏书 · ' + title)
        stand.set_folder_path('藏书')
        stand.set_editor_property('tags', [tag])
        stand.set_editor_property('book', asset)
        report.append({'room':room, 'title':title, 'pages':len(pages), 'thoughts':len(thoughts),
                       'position':[pos.x,pos.y,pos.z]})

    assert LEVELS.save_current_level()
    out = PROJECT/'Saved/RestLoreInstallation.json'
    out.write_text(json.dumps({'map':MAP, 'books':report, 'backup':str(backup)},ensure_ascii=False,indent=2),
                   encoding='utf-8')
    unreal.log('REST_LORE_INSTALLED')


command_line = unreal.SystemLibrary.get_command_line()
if '-RestLoreVerifyText' in command_line:
    update_existing_books(verify_only=True)
elif '-RestLoreTextOnly' in command_line:
    update_existing_books(verify_only=False)
else:
    install_reading_points()
