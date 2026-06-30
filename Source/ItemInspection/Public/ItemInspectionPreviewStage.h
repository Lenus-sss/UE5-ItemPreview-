#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemInspectionPreviewStage.generated.h"

class AActor;
class UDirectionalLightComponent;
class UItemInspectionConfigComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UTexture2D;
class UTextureRenderTarget2D;

/**
 * 物品检视预览舞台，负责复制模型、布光、拍摄 RenderTarget。
 */
UCLASS()
class ITEMINSPECTION_API AItemInspectionPreviewStage : public AActor
{
	GENERATED_BODY()

public:
	/** 创建预览根组件、捕获相机和灯光。 */
	AItemInspectionPreviewStage();

	/** 根据源 Actor 构建预览模型，并返回是否成功复制到至少一个网格组件。 */
	bool BuildPreviewFromActor(AActor* SourceActor);

	/** 获取当前预览画面使用的 RenderTarget。 */
	UTextureRenderTarget2D* GetPreviewRenderTarget() const { return PreviewRenderTarget; }

	/** 获取已经把透明通道转成 UMG 可用格式的显示贴图。 */
	UTexture2D* GetPreviewDisplayTexture() const { return PreviewDisplayTexture; }

	/** 根据鼠标拖拽增量旋转预览模型。 */
	void AddOrbitRotation(const FVector2D& MouseDelta, float DragRotationSpeed);

	/** 根据滚轮缩放相机距离。 */
	void AddCameraZoom(float WheelDelta, float WheelZoomSpeed);

	/** 捕获一次当前预览画面。 */
	void CapturePreview();

protected:
	/** Actor 销毁前清理运行时 RenderTarget 引用。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 清空上一次复制出来的预览组件。 */
	void ClearPreviewComponents();

	/** 在预览 Actor 上复制 StaticMeshComponent。 */
	int32 CopyStaticMeshComponents(AActor* SourceActor);

	/** 在预览 Actor 上复制 SkeletalMeshComponent。 */
	int32 CopySkeletalMeshComponents(AActor* SourceActor);

	/** 根据预览模型包围盒刷新中心、半径和相机距离。 */
	void RefreshPreviewBounds();

	/** 根据当前相机参数移动 SceneCapture。 */
	void UpdateCaptureCamera();

	/** 判断源组件是否应该复制。 */
	bool ShouldCopyMeshComponent(const UPrimitiveComponent* Component) const;

	/** 读取源 Actor 上的检视配置组件，覆盖默认相机参数。 */
	void ApplyInspectionConfig(const AActor* SourceActor);

	/** 根据模型尺寸刷新预览灯光位置、半径和亮度。 */
	void UpdatePreviewLighting();

	/** 创建或重建 UMG 显示用贴图。 */
	void CreateOrResizeDisplayTexture(int32 Width, int32 Height);

	/** 从 RenderTarget 读回像素，并把 UE 的反透明度 alpha 转成正常 alpha。 */
	void UpdateDisplayTextureFromRenderTarget();

	/** 舞台根组件，承载捕获相机和灯光。 */
	UPROPERTY(VisibleAnywhere, Category = "Item Inspection")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 预览模型根组件，拖拽旋转时只旋转它。 */
	UPROPERTY(VisibleAnywhere, Category = "Item Inspection")
	TObjectPtr<USceneComponent> PreviewRoot;

	/** 拍摄预览模型的 SceneCapture。 */
	UPROPERTY(VisibleAnywhere, Category = "Item Inspection")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	/** 预览模型用的主灯。 */
	UPROPERTY(VisibleAnywhere, Category = "Item Inspection")
	TObjectPtr<UPointLightComponent> KeyLight;

	/** 预览模型用的方向补光，保证大物体侧面不会完全黑。 */
	UPROPERTY(VisibleAnywhere, Category = "Item Inspection")
	TObjectPtr<UDirectionalLightComponent> DirectionalFillLight;

	/** 预览模型用的弱补光，减少背光面死黑。 */
	UPROPERTY(VisibleAnywhere, Category = "Item Inspection")
	TObjectPtr<UPointLightComponent> FillLight;

	/** 显示到 Widget Image 上的 RenderTarget。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	/** 显示到 Widget Image 上的贴图，alpha 已经转换为正常透明度。 */
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PreviewDisplayTexture;

	/** 运行时复制出来的预览组件。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPrimitiveComponent>> PreviewComponents;

	/** 是否只复制可见网格组件。 */
	UPROPERTY(EditAnywhere, Category = "Item Inspection")
	bool bOnlyCopyVisibleMeshComponents = true;

	/** RenderTarget 分辨率。 */
	UPROPERTY(EditAnywhere, Category = "Item Inspection")
	FIntPoint RenderTargetSize = FIntPoint(1024, 1024);

	/** 主灯强度。 */
	UPROPERTY(EditAnywhere, Category = "Item Inspection")
	float PreviewLightIntensity = 8000.0f;

	/** 自动计算相机距离后的默认额外倍率，避免长条物体贴脸。 */
	UPROPERTY(EditAnywhere, Category = "Item Inspection")
	float DefaultCameraDistanceScale = 1.35f;

	/** 相机允许靠近模型的最小距离。 */
	UPROPERTY(EditAnywhere, Category = "Item Inspection")
	float MinCameraDistance = 35.0f;

	/** 相机允许远离模型的最大距离。 */
	UPROPERTY(EditAnywhere, Category = "Item Inspection")
	float MaxCameraDistance = 3000.0f;

	/** 当前预览模型中心。 */
	FVector PreviewCenter = FVector::ZeroVector;

	/** 当前预览模型半径。 */
	float PreviewRadius = 50.0f;

	/** 当前相机距离倍率，默认值可被源 Actor 配置组件覆盖。 */
	float CameraDistanceScale = 1.35f;

	/** 当前相机距离额外偏移，默认值可被源 Actor 配置组件覆盖。 */
	float CameraDistanceOffset = 0.0f;

	/** 当前相机水平角。 */
	float CameraYaw = 180.0f;

	/** 当前相机俯仰角。 */
	float CameraPitch = -10.0f;

	/** 当前相机距离。 */
	float CameraDistance = 120.0f;
};
