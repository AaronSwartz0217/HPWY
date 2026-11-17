from pathlib import Path
for name in ['setup_chromatic_ink.py','configure_shanhe_spawning.py','configure_shanhe_chunks.py']:
    script=Path(__file__).with_name(name)
    exec(compile(script.read_text(encoding='utf-8-sig'),str(script),'exec'),{'__file__':str(script)})
