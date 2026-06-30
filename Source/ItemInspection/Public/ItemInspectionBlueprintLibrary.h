#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemInspectionBlueprintLibrary.generated.h"

class AActor;
class UItemInspectionWidget;

/**
 * 物品检视蓝图调用入口，用一个节点打开可旋转查看模型的检视 Widget。
 */
UCLASS()
class ITEMINSPECTION_API UItemInspectionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 创建检视 Widget，把 Actor 的模型复制进去，并显示到指定屏幕位置。 */
	UFUNCTION(BlueprintCallable, Category = "Item Inspection", meta = (WorldContext = "WorldContextObject", DisplayName = "Open Item Inspection"))
	static UItemInspectionWidget* OpenItemInspection(const UObject* WorldContextObject, AActor* ActorToInspect, const FVector2D& ScreenPosition, TSubclassOf<UItemInspectionWidget> WidgetClass, int32 PlayerIndex = 0, int32 ZOrder = 100);
};
