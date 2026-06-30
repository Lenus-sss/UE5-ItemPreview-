#include "ItemInspection.h"

#define LOCTEXT_NAMESPACE "FItemInspectionModule"

void FItemInspectionModule::StartupModule()
{
	// 插件当前只提供运行时 Widget 和蓝图函数，不需要在模块启动时注册额外资源。
}

void FItemInspectionModule::ShutdownModule()
{
	// 插件当前没有全局缓存资源，模块关闭时保持空实现即可。
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FItemInspectionModule, ItemInspection)
