#pragma once

#include "Modules/ModuleManager.h"

/**
 * ItemInspection 插件模块入口，负责让 UE 在运行时加载物品检视功能。
 */
class FItemInspectionModule : public IModuleInterface
{
public:
	/** 模块启动时调用，目前插件没有额外全局资源需要初始化。 */
	virtual void StartupModule() override;

	/** 模块关闭时调用，目前插件没有额外全局资源需要释放。 */
	virtual void ShutdownModule() override;
};
