#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "ItemInspectionWidget.generated.h"

class AActor;
class APlayerController;
class AItemInspectionPreviewStage;
class UButton;
class UImage;

/** 检视窗口关闭时广播，方便蓝图同步恢复玩家输入或 UI 状态。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FItemInspectionClosedSignature);

/**
 * 物品检视 Widget 基类；你的蓝图 Widget 继承它，并放置 InspectImage 和 CloseButton 即可。
 */
UCLASS(BlueprintType, Blueprintable)
class ITEMINSPECTION_API UItemInspectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 配置检视目标，并把外层 Widget 放到视口中心；传入位置只作为视口尺寸无效时的兜底。 */
	UFUNCTION(BlueprintCallable, Category = "Item Inspection")
	void SetupInspection(AActor* InSourceActor, const FVector2D& InScreenPosition);

	/** 关闭检视窗口，同时清理运行时生成的预览模型和捕获相机。 */
	UFUNCTION(BlueprintCallable, Category = "Item Inspection")
	void CloseInspection();

	/** 检视窗口被主动关闭时触发。 */
	UPROPERTY(BlueprintAssignable, Category = "Item Inspection")
	FItemInspectionClosedSignature OnInspectionClosed;

protected:
	/** Widget 创建完成时调用，此时蓝图里的绑定控件已经可以使用。 */
	virtual void NativeConstruct() override;

	/** Widget 销毁时调用，确保预览 Actor 不会留在关卡世界里。 */
	virtual void NativeDestruct() override;

	/** 每帧检查延迟创建请求，并在存在捕获相机时刷新 RenderTarget。 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 鼠标左键按下时开始拖拽旋转模型。 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 鼠标移动时根据拖拽距离旋转预览相机。 */
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 鼠标左键松开时结束拖拽旋转。 */
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 鼠标滚轮滚动时缩放预览相机距离。 */
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** 响应关闭按钮点击，转到统一的关闭逻辑。 */
	UFUNCTION()
	void HandleCloseButtonClicked();

	/** 尝试从源 Actor 创建预览舞台，并把舞台 RenderTarget 显示到 Image 上。 */
	bool TryBuildPreviewStageFromSourceActor();

	/** 创建 RenderTarget，并把它设置到蓝图里的 Image 控件上。 */
	bool SetupRenderTargetBrush(UObject* RenderTargetResource);

	/** 打开检视窗口时切到 UI 输入模式，显示鼠标并锁定到视口。 */
	void SetupOwningPlayerInput();

	/** 关闭检视窗口时恢复游戏输入模式和鼠标显示状态。 */
	void RestoreOwningPlayerInput();

	/** 销毁预览 Actor，并重置相机和拖拽状态。 */
	void CleanupPreview();

	/** 你在 Widget Blueprint 中放置的 Image，命名为 InspectImage 即可自动绑定。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item Inspection", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> InspectImage;

	/** 你在 Widget Blueprint 中放置的关闭按钮，命名为 CloseButton 即可自动绑定。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item Inspection", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> CloseButton;

	/** 是否只复制当前可见的网格组件，避免把隐藏辅助模型也放进检视窗口。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection", meta = (AllowPrivateAccess = "true"))
	bool bOnlyCopyVisibleMeshComponents = true;

	/** 鼠标拖拽时每个屏幕像素对应的旋转角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float DragRotationSpeed = 0.35f;

	/** 鼠标滚轮缩放速度，数值越大缩放越快。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection", meta = (ClampMin = "0.01", AllowPrivateAccess = "true"))
	float WheelZoomSpeed = 0.12f;

	/** 检视 Widget 放到屏幕中心时使用的默认尺寸。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection", meta = (AllowPrivateAccess = "true"))
	FVector2D WidgetDesiredSize = FVector2D(1013.0f, 760.0f);

	/** 预览 Image 在 CanvasPanel 中的层级，默认压过背景图。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Inspection", meta = (AllowPrivateAccess = "true"))
	int32 PreviewImageZOrder = 10;

	/** 等待复制的源 Actor，使用弱引用避免阻止关卡 Actor 正常销毁。 */
	TWeakObjectPtr<AActor> SourceActor;

	/** 运行时生成的预览舞台，关闭 Widget 时会被销毁。 */
	UPROPERTY(Transient)
	TObjectPtr<AItemInspectionPreviewStage> PreviewStage;

	/** 打开检视窗口时使用的玩家控制器，用于关闭时恢复输入模式。 */
	UPROPERTY(Transient)
	TObjectPtr<APlayerController> CachedOwningPlayer;

	/** 打开检视窗口前玩家控制器是否显示鼠标。 */
	bool bPreviousShowMouseCursor = false;

	/** 当前是否已经接管过玩家输入，避免重复恢复。 */
	bool bHasAppliedInspectionInputMode = false;

	/** 当前是否还需要创建预览模型。 */
	bool bPendingPreviewBuild = false;

	/** 当前是否已经执行过关闭逻辑，防止重复广播关闭事件。 */
	bool bClosed = false;

	/** 当前是否正在主动关闭检视窗口，用于区分按钮关闭和异常析构。 */
	bool bIsClosingInspection = false;

	/** 当前是否正在用鼠标拖拽旋转。 */
	bool bDraggingPreview = false;

	/** 上一次鼠标屏幕坐标，用来计算拖拽增量。 */
	FVector2D LastMouseScreenPosition = FVector2D::ZeroVector;

};
