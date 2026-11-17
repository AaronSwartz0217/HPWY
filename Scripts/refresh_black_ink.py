from pathlib import Path
for name in ['setup_chromatic_ink.py','inspect_ink_transform.py']:
    script=Path(__file__).with_name(name)
    exec(compile(script.read_text(encoding='utf-8-sig'),str(script),'exec'),{'__file__':str(script)})
