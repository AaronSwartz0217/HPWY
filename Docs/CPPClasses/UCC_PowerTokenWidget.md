# UCC_PowerTokenWidget

父类UUserWidget。格子背包的单个信物控件，由UCC_MuralMenuWidget自动创建，不需要手动放置到关卡。

- `SetEntry(Form, Tier, bActive, bSelected, Art)`：更新信物、档位、使用中和选中状态。Form为空表示禁用空格。
- `GetEntryForm()`：读取该格对应的力量。
- `OnSelected(Form)`：点击回调，仅表示选择；主菜单再提供使用/进化操作。

原生格子尺寸142×172，支持鼠标点击与按钮焦点。边框读取Art.SkillGallery.CardNormal/CardSelected；优先显示Form.InventoryIcon，没有图时显示墨滴/铜符/陶印/竹节占位图形。名称读取Form.GetInventoryName，档位从Progression.GetOwnedFormTier取得。信物本身不直接扣钱或执行服务器成长交易。

NativeOnInitialized先建立控件树，之后SetEntry赋值并刷新。若要换图，修改对应FormDefinition的InventoryIcon即可；现有原生结构不需要Designer蓝图。

[完整配置](../格子背包与初墨成长配置.md) · [父类索引](../CPP父类配置索引.md)
