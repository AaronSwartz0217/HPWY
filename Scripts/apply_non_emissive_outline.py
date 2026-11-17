"""Apply the non-emissive outline and render its two variants in the existing preview map."""
from pathlib import Path
import runpy
folder=Path(__file__).resolve().parent
runpy.run_path(str(folder/'setup_chromatic_ink.py'),run_name='__main__')
runpy.run_path(str(folder/'capture_chromatic_ink.py'),run_name='__main__')
