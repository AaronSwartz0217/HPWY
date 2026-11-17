"""Run both post-build asset setup stages inside one Unreal editor commandlet."""
from pathlib import Path
import runpy
folder=Path(__file__).resolve().parent
runpy.run_path(str(folder/'configure_consumables_and_copper.py'),run_name='__main__')
runpy.run_path(str(folder/'import_consumable_icons.py'),run_name='__main__')
