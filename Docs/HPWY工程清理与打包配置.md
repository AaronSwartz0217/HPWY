# HPWY 工程清理与打开方式

当前项目描述文件为 HPWY.uproject，构建目标为 HPWY（Game）与 HPWYEditor。共享代码模块 CrashCourse 保留，以兼容蓝图序列化的 /Script/CrashCourse 路径。工程文件夹暂保留 HPWH，Content/HPWH 是现用资源包路径，均不应仅因名称不同而删除。

打包配置 BuildTarget=HPWY，只包含现用地图 /Game/HPWH/Maps/L_HPWH_Prototype。打开工程请选择 HPWY.uproject；如编辑器原来开着旧的 HPWH.uproject，关闭后改开新文件。打包日志目标应为 -target=HPWY。

## 清理范围

367 个不在现用关卡、动态目录和原生加载依赖闭包中的资产已移出 Content，包含旧测试地图、主菜单地图和预览图。此前退役的小怪资产与其他备份目录也已移至工程同级目录 HPWY_工程外备份_20261119。CleanupPlan.json 记录本轮保留和移出清单。没有永久销毁原资源。

保留现用主角、徘徊者、五铢钱、道具、音频、UI，以及原生构造函数仍加载的共享框架依赖。保留 C++ 源码和插件；本轮没有为了缩小目录而破坏旧类的编译接口。

如需恢复，先关闭编辑器，按 RetiredContent 中相对 Content 的路径复制所需资产及其依赖回工程。NamingBefore 保存旧 Target 与工程描述文件，不能在现有 Source 中同时放入同名目标。

新打包命令示例：
RunUAT.bat BuildCookRun -project=C:/HPWHBuild/HPWH/HPWY.uproject -target=HPWY -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive -archivedirectory=C:/HPWY_Package -nop4 -utf8output

该目录联接指向同一工程，用 ASCII 路径规避既往的中文路径编译问题。正式归档目录由使用者自行指定；本轮未生成完整发布包。
