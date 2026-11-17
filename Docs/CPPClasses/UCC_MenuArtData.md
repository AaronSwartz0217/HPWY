# UCC_MenuArtData

父类：`UDataAsset`。编辑器名称：`CC_MenuArtData`。核对日期：2026-10-24。

[头文件](../../Source/CrashCourse/Public/UI/CC_MenuArtData.h) · [返回索引](../CPP父类配置索引.md)

## 用途与当前实例

保存壁画菜单共用美术引用。已生成实例 `/Game/UI/MuralMenu/DA_MenuArt`，由 `WBP_MuralMenu → Art Data` 引用。它只提供美术资源，不记录经验、已解锁元素、进化档位或技能装备。

## 配置方式

在 Content Browser 创建 Data Asset，类型选 `CC_MenuArtData`；或直接编辑现有 `DA_MenuArt`。

| 字段 | 用途与当前默认值 |
|---|---|
| `Background` | 全屏壁画背景，`T_Menu_Background` |
| `Logo` | 左上 Logo，`T_Menu_Logo` |
| `Navigation` | 四项导航图片，每项包含 `Normal` 和 `Active` |
| `FallbackPortrait` | 形态未配置立绘时使用 `T_Portrait_Ink_01` |
| `FallbackTitle` | 形态未配置称号时使用 `T_Title_Ink` |
| `WebsiteURL` | 独立首页宣传网站按钮使用的 HTTP/HTTPS 地址 |
| `StartupLogo` | 独立首页开场黑底Logo图片，2秒保持+1秒淡出 |

`Navigation` 为固定顺序：0 背包、1 技能、2 交易、3 设置。`FCC_MenuNavigationArt.Normal` 是普通态，`Active` 用于选中、悬停和按下。数组位置决定导航用途，不能按中文名称重新排序。每项两张图都要填写。

全部图片为 Texture2D 软引用，菜单打开时按需加载。各形态正式图片不放在此资产：应编辑对应 `DA_Form_* → Stages[n] → MenuPortrait / MenuTitle / MenuDisplayName`。

首页另用 `/Game/UI/MainMenu/DA_HomeArt`，Background 为第一版四派系壁画，Logo 复用现有资源；WebsiteURL 目前为项目 GitHub 地址，SkillGallery 留空。修改首页这份资产不影响游戏内 DA_MenuArt。[主页配置](../游戏主页与背景音乐配置.md)。

## 图片与维护

纹理使用 UserInterface2D (RGBA8)、UI 组、NoMipmaps、sRGB，保留 Alpha。使用独立角色壁画和竖版称号，不使用包含多个人物的生成原稿。菜单按原图比例显示。

批量导入入口为 `Scripts/import_mural_menu_assets.py`，来源清单在 `Saved/UI/MuralMenuImport.json`。重跑会恢复脚本中配置的共用图片及 Stage 绑定；手工调整后应同步脚本映射。

## 联调检查

Tab 应打开已有 `WBP_MuralMenu`：背景覆盖 viewport，四个按钮显示对应美术；当前形态立绘在右侧，称号在右上方。分别切换形态与档位验证 Stage 图片，留空某个 Stage 图片时检查初墨回退。

本轮构建、5 项核心测试、184 项菜单实测检查与实际截图核对通过。详细操作见 [壁画菜单资源配置](../壁画菜单资源配置.md)。

新增 SkillGallery 指向独立的18项技能美术目录。参见 [技能界面美术配置](../技能界面美术配置.md)。
