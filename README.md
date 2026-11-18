# 幻魄物语 · PHANTOM SOUL OF THE MANOR

一款使用 **Unreal Engine 5.6 / C++** 制作的俯视角探索解谜与轻 Roguelite 生存游戏。

尤里西斯回到英国祖宅，在尘封的器物与旧日记忆之间坠入梦境。玩家需要探索不断延展的梦中案头，完成三类器物谜题，在怪物与徘徊者的追逐中寻找异层传送门，并设法从梦中醒来。

- **当前版本**：v1.0
- **宣传官网**：https://aaronswartz0217.github.io/HPWY/
- **源码仓库**：https://github.com/AaronSwartz0217/HPWY
- **更新日期**：2026-11-19

## 当前玩法

- **程序化梦境**：确定性格局生成、玩家周围 5×5 活动格流送，以及基于 SDF 的连续群系分布。
- **三类器物谜题**：八卦镜、四合院模型与笔记本电脑；每类首次完成后推进本层目标。
- **轻 Roguelite 成长**：击杀与解谜获得三选一机会，17 种强化按普通、中级、极品稀有度出现，每组可刷新一次。
- **战斗与追逐**：自动瞄准魂灯弹、近远程器物怪、摄魂蜘蛛、五铢钱怪与持续施压的徘徊者。
- **灵魂离体**：被摄魂蜘蛛击中后控制灵魂寻找肉身；离体期间无法攻击、解谜或进入传送门。
- **同伴与叛变**：可邀请同伴同行，但受伤可能触发叛变，将协助关系转化为新的战斗压力。
- **异层传送门**：完成三类谜题后生成。初始通关概率为 15%，进入下一层时基础概率翻倍，死亡等因素会影响最终概率。
- **完整界面流程**：主界面、全屏梦境档案、强化卡牌、传送门红卡、设置与本地存档。
- **动态音频**：探索、警戒与徘徊者接近使用不同音乐状态，并配有战斗、解谜和界面音效。

## 操作方式

| 输入 | 功能 |
|---|---|
| `W / A / S / D` | 移动 |
| 鼠标左键 | 攻击 |
| 鼠标右键 / 鼠标移动 | 调整观察与瞄准方向 |
| `E` | 与器物、同伴、肉身或传送门交互 |
| `1 / 2 / 3` | 选择强化卡或菜单项 |
| `R` | 刷新当前一组强化卡 |
| `Esc / Backspace` | 关闭当前面板或返回 |
| 触屏摇杆与右侧按钮 | 移动端移动、攻击与交互 |

## 运行工程

### 环境要求

- Windows 10 / 11
- Unreal Engine **5.6**
- Visual Studio 2022（含 C++ 游戏开发组件）
- Git 与 Git LFS

### 克隆与启动

```bash
git lfs install
git clone git@github.com:AaronSwartz0217/HPWY.git
cd HPWY
git lfs pull
```

1. 使用 UE 5.6 打开根目录的 `HPWY.uproject`。
2. 按提示编译 `CrashCourse` C++ 模块，或使用 Visual Studio 构建 `HPWYEditor`。
3. 打开 `/Game/HPWH/Maps/L_HPWH_Prototype`，点击 **Play**。

仓库不包含 `Binaries/`、`Intermediate/`、`Saved/` 与 `DerivedDataCache/`，这些目录由 Unreal Engine 重新生成。DLSS 插件也未纳入版本库；需要时请在本地安装，默认可使用 TSR。

## 打包

仓库提供 `Scripts/package_hpwy.ps1`，通过 Unreal Automation Tool 执行 BuildCookRun。

```powershell
.\Scripts\package_hpwy.ps1 `
  -Platform Win64 `
  -Project "C:\path\to\HPWY.uproject" `
  -ArchiveDirectory "C:\HPWY_Packages"
```

脚本默认使用 `C:\Program Files\Epic Games\UE_5.6`；其他安装位置需相应调整。Android 配置已包含 Arm64、ASTC、横屏与 APK 内嵌数据选项，打包前还需安装 UE Android 平台组件，并配置 JDK 17、Android SDK 34 与 NDK 25.1.8937393。

## 目录结构

```text
HPWY/
├── Config/                         # 项目、输入、打包与平台配置
├── Content/HPWH/                   # 地图、模型、材质、音频与剧情资源
├── Source/
│   ├── CrashCourse/Public/HPWH/    # 当前玩法的 C++ 头文件
│   ├── CrashCourse/Private/HPWH/   # 世界、角色、敌人、谜题与 UI 实现
│   ├── HPWY.Target.cs              # 游戏构建目标
│   └── HPWYEditor.Target.cs        # 编辑器构建目标
├── Scripts/                        # 打包与资源处理脚本
├── Docs/                           # 早期设计与开发过程文档
└── HPWY.uproject                   # UE 5.6 工程入口
```

核心代码位于 `Source/CrashCourse`；`HPWH` 子目录和资源路径为当前工程沿用的内部命名。`Docs/` 中部分《物魄奇谭》文档属于早期设计与历史记录，当前版本行为以代码、配置及宣传官网为准。

## 仓库分支

- `master`：Unreal Engine 源码、配置和资源。
- `gh-pages`：宣传官网静态页面，访问 https://aaronswartz0217.github.io/HPWY/ 。

## 许可证

本项目仅供学习、比赛评审与研究使用。
