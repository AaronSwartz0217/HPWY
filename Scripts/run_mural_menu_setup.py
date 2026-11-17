import runpy, time, unreal
from pathlib import Path
project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
try:
    runpy.run_path(str(project/'Scripts/configure_initial_ink_nature.py'),run_name='__main__')
    runpy.run_path(str(project/'Scripts/import_mural_menu_assets.py'),run_name='__main__')
except Exception:
    import traceback
    unreal.log_error(traceback.format_exc())
    unreal.SystemLibrary.quit_editor()
else:
    quit_at=time.monotonic()+5
    def done(delta):
        if time.monotonic()>quit_at:
            unreal.unregister_slate_post_tick_callback(handle)
            unreal.SystemLibrary.quit_editor()
    handle=unreal.register_slate_post_tick_callback(done)
