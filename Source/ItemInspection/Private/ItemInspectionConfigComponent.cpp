#include "ItemInspectionConfigComponent.h"

UItemInspectionConfigComponent::UItemInspectionConfigComponent()
{
	// 配置组件只提供 Actor 级参数，不参与运行时更新。
	PrimaryComponentTick.bCanEverTick = false;
}
