using UnrealBuildTool;

// 定义 ItemInspection 模块依赖，让插件可以创建 UMG Widget 并用 SceneCapture 渲染预览模型。
public class ItemInspection : ModuleRules
{
	// 构造模块规则，由 Unreal Build Tool 在生成项目和编译时调用。
	public ItemInspection(ReadOnlyTargetRules Target) : base(Target)
	{
		// 使用显式或共享预编译头，保持插件编译方式和 UE 常规 C++ 模块一致。
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 公开依赖供外部模块调用本插件公开头文件时使用。
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"SlateCore"
		});

		// 私有依赖只给本模块实现文件使用，主要用于 Slate 鼠标事件和控件底层逻辑。
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Slate"
		});
	}
}
