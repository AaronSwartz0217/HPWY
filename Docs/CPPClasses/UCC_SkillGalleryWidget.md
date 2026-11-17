# UCC_SkillGalleryWidget

UMG UserWidget父类，由MuralMenu按ArtData.SkillGallery创建。四分类、图标卡片、右侧说明、底部真实四槽按1920×1080等比布局。

- SelectCategory/SelectEntry选择资料，SelectPreviewSlot选择实际目标槽。
- PreviewEquip为兼容保留旧名，当前请求服务器EquipSkill；不会提前修改假图标。
- RefreshFromPlayer读取CurrentSlots、实际键位和解锁状态；HandleEquipmentResult显示权威结果。
- ResetEquipment恢复当前实际形态默认组；手动组跨形态保留。
- 未解锁项灰显，冷却/施法时不能换装；实际服务器和GAS仍再次校验。
- 原22模板若无18项图标映射，槽中显示真实名称，避免假装空槽。

详见[技能美术配置](../技能界面美术配置.md)。
