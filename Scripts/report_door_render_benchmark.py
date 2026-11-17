import json
from pathlib import Path
p=Path(__file__).resolve().parents[1];first=json.loads((p/'Saved/DoorRenderBenchmark_20261103/summary.json').read_text(encoding='utf-8'));second=json.loads((p/'Saved/DoorRenderBenchmark_20261103_Reverse/summary.json').read_text(encoding='utf-8'))
a=second['phases']['on_before'];b=second['phases']['on_after'];off=second['phases']['off'];n=a['frames']+b['frames']
def avg(k):return (a['metrics'][k]['mean']*a['frames']+b['metrics'][k]['mean']*b['frames'])/n
r={'conditions':'same map, same process off/on/off then on/off/on; outdoor spawn; doors closed; output1920x1080 screenpercentage100; 250warmup and50tail frames excluded per phase','reference_off_ms':off['metrics']['FrameTime']['mean'],'pooled_on_ms':avg('FrameTime'),'estimated_total_saved_ms':off['metrics']['FrameTime']['mean']-avg('FrameTime'),'off_gpu_ms':off['metrics']['GPUTime']['mean'],'on_gpu_ms':avg('GPUTime'),'off_drawcalls':off['metrics']['RHI/DrawCalls']['mean'],'on_drawcalls':avg('RHI/DrawCalls'),'conclusion':'Consistent GPU saving about0.4ms and drawcall reduction about35%. Total frame improvement is not confidently established because baseline/frame-time drift exceeds the estimated gain. Fixed-view benchmark only, not whole-level60fps verification.','first_run':first,'reverse_run':second}
(p/'Saved/DoorRenderBenchmark_20261103/combined_report.json').write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in r.items() if k not in ['first_run','reverse_run']},ensure_ascii=False,indent=2))
