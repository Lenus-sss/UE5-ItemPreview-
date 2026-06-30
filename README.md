![Item Inspection Demo](Show/Ins.gif)

# UE5-ItemInspection

Runtime UE5 plugin for inspecting actor meshes in a centered UMG preview window.

## What It Does

- Opens a centered item inspection window at runtime.
- Copies visible `StaticMeshComponent` / `SkeletalMeshComponent` from an Actor.
- Renders the copied mesh to a UMG `Image`.
- Supports mouse drag rotation and wheel zoom.

## Install

Put the plugin here:

```text
YourProject/Plugins/ItemInspection
```

Restart Unreal Editor and compile the project.

## Create The Widget

Create a `Widget Blueprint`:

```text
Parent Class: ItemInspectionWidget
```

Required widget names:

```text
InspectImage
CloseButton
```

Recommended hierarchy:

```text
Canvas Panel
  BackgroundImage
  InspectImage
  CloseButton
```

`InspectImage` displays the preview render. `CloseButton` closes the inspection window.

## Open Inspection

In Blueprint, call:

```text
Open Item Inspection
```

Main parameters:

```text
Actor To Inspect   Actor shown in the preview
Widget Class       Your ItemInspectionWidget blueprint
Player Index       Usually 0
Z Order            UI layer
Screen Position    Fallback only; the window is centered by default
```

## Controls

```text
Left Mouse Drag    Rotate preview
Mouse Wheel        Zoom preview
CloseButton        Close window
```

## Optional Actor Config

Add `ItemInspectionConfigComponent` to the inspected Actor when a model needs custom framing.

Useful properties:

```text
Camera Distance Scale
Camera Distance Offset
Camera FOV Override
Min / Max Camera Distance Override
```

## Repository

```text
Name: UE5-ItemInspection
Description: Runtime UE5 plugin for inspecting actor meshes in a centered UMG preview window.
```
