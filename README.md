![物品预览演示](Show/Ins.gif)

# UE5 Item Preview / UE5 物品预览

UE5 运行时物品预览插件。  
可以把关卡里的 Actor 模型显示到 UMG 窗口里，支持旋转、缩放和关闭。

## 功能

- 运行时打开居中的预览窗口。
- 复制 Actor 上可见的 `StaticMeshComponent` / `SkeletalMeshComponent`。
- 把模型渲染到 UMG 的 `Image` 控件。
- 支持鼠标拖拽旋转、滚轮缩放。

## 安装

把插件放到项目目录：

```text
YourProject/Plugins/ItemInspection
```

重启 UE，然后编译项目。

## 创建预览窗口

新建一个 `Widget Blueprint`：

```text
Parent Class: ItemInspectionWidget
```

必须有这两个控件，并且名字要完全一致：

```text
InspectImage
CloseButton
```

推荐层级：

```text
Canvas Panel
  BackgroundImage
  InspectImage
  CloseButton
```

说明：

```text
InspectImage  显示模型预览
CloseButton   关闭预览窗口
```

## 打开预览

在蓝图里调用：

```text
Open Item Inspection
```

常用参数：

```text
Actor To Inspect   要预览的 Actor
Widget Class       你的预览窗口 Widget
Player Index       单机通常填 0
Z Order            UI 层级
Screen Position    兜底位置；默认会居中显示
```

## 操作

```text
鼠标左键拖拽    旋转模型
鼠标滚轮        缩放模型
CloseButton     关闭窗口
```

## 调整模型显示

如果某个模型太大、太小或角度不合适，可以给它添加：

```text
ItemInspectionConfigComponent
```

常用参数：

```text
Camera Distance Scale          调整默认远近
Camera Distance Offset         额外增加距离
Camera FOV Override            覆盖相机视角
Min / Max Camera Distance      限制滚轮缩放范围
```

## 仓库信息

```text
名称：UE5 Item Preview / UE5 物品预览
简介：UE5 运行时物品预览插件，支持在 UMG 窗口中查看、旋转和缩放 Actor 模型。
```
