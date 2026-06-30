#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemInspectionConfigComponent.generated.h"

/**
 * 物品检视配置组件，挂到被检视 Actor 上后会覆盖默认预览相机参数。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup = (ItemInspection), meta = (BlueprintSpawnableComponent))
class ITEMINSPECTION_API UItemInspectionConfigComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 创建配置组件；本组件只保存数据，不需要 Tick。 */
	UItemInspectionConfigComponent();

	/** 相机自动适配包围盒后的额外距离倍率，数值越大物体越小、越容易看全貌。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection|Camera", meta = (ClampMin = "0.1"))
	float CameraDistanceScale = 1.35f;

	/** 在自动计算距离后额外增加的世界单位距离，适合特别长或特别宽的物体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection|Camera")
	float CameraDistanceOffset = 0.0f;

	/** 覆盖预览相机 FOV；小于等于 0 时使用插件默认 FOV。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection|Camera")
	float CameraFOVOverride = 0.0f;

	/** 覆盖最近相机距离；小于等于 0 时使用插件默认最小距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection|Camera")
	float MinCameraDistanceOverride = 0.0f;

	/** 覆盖最远相机距离；小于等于 0 时使用插件默认最大距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection|Camera")
	float MaxCameraDistanceOverride = 0.0f;
};
