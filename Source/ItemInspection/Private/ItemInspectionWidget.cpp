#include "ItemInspectionWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "ItemInspectionPreviewStage.h"
#include "Styling/SlateBrush.h"

void UItemInspectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 关闭按钮由蓝图 Widget 提供，命名为 CloseButton 后会自动绑定到这里。
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UItemInspectionWidget::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UItemInspectionWidget::HandleCloseButtonClicked);
	}
}

void UItemInspectionWidget::NativeDestruct()
{
	// PIE 停止或 Widget 被外部移除时也要释放预览 Actor，并恢复玩家输入。
	UE_LOG(LogTemp, Log, TEXT("ItemInspectionWidget: NativeDestruct 触发，bIsClosingInspection=%s。"), bIsClosingInspection ? TEXT("true") : TEXT("false"));
	CleanupPreview();
	RestoreOwningPlayerInput();

	Super::NativeDestruct();
}

void UItemInspectionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 有些蓝图控件会在下一帧才完成布局，这里允许延后一帧再绑定 RenderTarget。
	if (bPendingPreviewBuild)
	{
		TryBuildPreviewStageFromSourceActor();
	}
}

FReply UItemInspectionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDraggingPreview = true;
		LastMouseScreenPosition = InMouseEvent.GetScreenSpacePosition();
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UItemInspectionWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDraggingPreview && PreviewStage)
	{
		const FVector2D CurrentMousePosition = InMouseEvent.GetScreenSpacePosition();
		const FVector2D MouseDelta = CurrentMousePosition - LastMouseScreenPosition;
		LastMouseScreenPosition = CurrentMousePosition;

		// 旋转和重新捕获都交给 PreviewStage，Widget 只负责把鼠标输入传过去。
		PreviewStage->AddOrbitRotation(MouseDelta, DragRotationSpeed);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UItemInspectionWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDraggingPreview)
	{
		bDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UItemInspectionWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (PreviewStage)
	{
		// 滚轮只调整预览相机距离，不影响玩家相机。
		PreviewStage->AddCameraZoom(InMouseEvent.GetWheelDelta(), WheelZoomSpeed);
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void UItemInspectionWidget::SetupInspection(AActor* InSourceActor, const FVector2D& InScreenPosition)
{
	SourceActor = InSourceActor;
	bClosed = false;
	bIsClosingInspection = false;

	// 默认使用传入位置兜底；正常运行时会用当前玩家视口中心覆盖它。
	FVector2D ViewportCenter = InScreenPosition;
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		OwningPlayer->GetViewportSize(ViewportSizeX, ViewportSizeY);
		if (ViewportSizeX > 0 && ViewportSizeY > 0)
		{
			ViewportCenter = FVector2D(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
		}
	}

	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetPositionInViewport(ViewportCenter, true);
	SetDesiredSizeInViewport(WidgetDesiredSize);
	ForceLayoutPrepass();

	SetupOwningPlayerInput();
	CleanupPreview();
	bPendingPreviewBuild = SourceActor.IsValid();

	if (!SourceActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionWidget: SourceActor 无效，无法创建预览模型。"));
		return;
	}

	TryBuildPreviewStageFromSourceActor();
}

void UItemInspectionWidget::CloseInspection()
{
	if (bClosed)
	{
		return;
	}

	bClosed = true;
	bIsClosingInspection = true;
	OnInspectionClosed.Broadcast();
	CleanupPreview();
	RestoreOwningPlayerInput();
	RemoveFromParent();
}

void UItemInspectionWidget::HandleCloseButtonClicked()
{
	CloseInspection();
}

bool UItemInspectionWidget::TryBuildPreviewStageFromSourceActor()
{
	AActor* SourceActorPtr = SourceActor.Get();
	if (!SourceActorPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionWidget: SourceActor 已失效，延迟创建预览失败。"));
		bPendingPreviewBuild = false;
		return false;
	}

	if (!InspectImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionWidget: 未找到名为 InspectImage 的 Image 控件。"));
		return false;
	}

	UWorld* World = SourceActorPtr->GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionWidget: SourceActor 没有有效 World。"));
		bPendingPreviewBuild = false;
		return false;
	}

	CleanupPreview();

	// 预览舞台放在源 Actor 上方较远处，避免和关卡里的原物体重叠。
	const FVector StageLocation = SourceActorPtr->GetActorLocation() + FVector(0.0f, 0.0f, 1000.0f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = NAME_None;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PreviewStage = World->SpawnActor<AItemInspectionPreviewStage>(
		AItemInspectionPreviewStage::StaticClass(),
		StageLocation,
		FRotator::ZeroRotator,
		SpawnParameters);

	if (!PreviewStage)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionWidget: 创建 PreviewStage 失败。"));
		bPendingPreviewBuild = false;
		return false;
	}

	if (!PreviewStage->BuildPreviewFromActor(SourceActorPtr))
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionWidget: PreviewStage 未能从 %s 构建预览模型。"), *SourceActorPtr->GetName());
		CleanupPreview();
		bPendingPreviewBuild = false;
		return false;
	}

	UObject* PreviewResource = PreviewStage->GetPreviewDisplayTexture() ? Cast<UObject>(PreviewStage->GetPreviewDisplayTexture()) : Cast<UObject>(PreviewStage->GetPreviewRenderTarget());
	if (!SetupRenderTargetBrush(PreviewResource))
	{
		CleanupPreview();
		bPendingPreviewBuild = false;
		return false;
	}

	bPendingPreviewBuild = false;
	return true;
}

bool UItemInspectionWidget::SetupRenderTargetBrush(UObject* RenderTargetResource)
{
	if (!InspectImage || !RenderTargetResource)
	{
		return false;
	}

	FVector2D BrushSize(1024.0f, 1024.0f);
	if (const UTextureRenderTarget2D* RenderTarget = Cast<UTextureRenderTarget2D>(RenderTargetResource))
	{
		BrushSize = FVector2D(RenderTarget->SizeX, RenderTarget->SizeY);
	}
	else if (const UTexture2D* Texture = Cast<UTexture2D>(RenderTargetResource))
	{
		BrushSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
	}

	FSlateBrush PreviewBrush;
	PreviewBrush.DrawAs = ESlateBrushDrawType::Image;
	PreviewBrush.SetResourceObject(RenderTargetResource);
	PreviewBrush.ImageSize = BrushSize;

	InspectImage->SetBrush(PreviewBrush);
	InspectImage->SetVisibility(ESlateVisibility::Visible);
	InspectImage->SetRenderOpacity(1.0f);
	InspectImage->SetColorAndOpacity(FLinearColor::White);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(InspectImage->Slot))
	{
		CanvasSlot->SetZOrder(PreviewImageZOrder);
		UE_LOG(LogTemp, Log, TEXT("ItemInspectionWidget: InspectImage 使用 CanvasPanelSlot，ZOrder=%d，Opacity=%.2f。"),
			PreviewImageZOrder,
			InspectImage->GetRenderOpacity());
	}

	UE_LOG(LogTemp, Log, TEXT("ItemInspectionWidget: 已绑定 RenderTarget 到 InspectImage，Resource=%s，Size=%s。"),
		*GetNameSafe(RenderTargetResource),
		*BrushSize.ToString());
	return true;
}

void UItemInspectionWidget::SetupOwningPlayerInput()
{
	CachedOwningPlayer = GetOwningPlayer();
	if (!CachedOwningPlayer)
	{
		return;
	}

	bPreviousShowMouseCursor = CachedOwningPlayer->bShowMouseCursor;
	CachedOwningPlayer->bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	CachedOwningPlayer->SetInputMode(InputMode);
	bHasAppliedInspectionInputMode = true;

	UE_LOG(LogTemp, Log, TEXT("ItemInspectionWidget: 已切换到 UIOnly 输入模式并显示鼠标。"));
}

void UItemInspectionWidget::RestoreOwningPlayerInput()
{
	if (!bHasAppliedInspectionInputMode || !CachedOwningPlayer)
	{
		return;
	}

	CachedOwningPlayer->bShowMouseCursor = bPreviousShowMouseCursor;

	FInputModeGameOnly InputMode;
	CachedOwningPlayer->SetInputMode(InputMode);
	bHasAppliedInspectionInputMode = false;
	CachedOwningPlayer = nullptr;

	UE_LOG(LogTemp, Log, TEXT("ItemInspectionWidget: 已恢复 GameOnly 输入模式。"));
}

void UItemInspectionWidget::CleanupPreview()
{
	bPendingPreviewBuild = false;
	bDraggingPreview = false;

	if (PreviewStage)
	{
		PreviewStage->Destroy();
		PreviewStage = nullptr;
	}
}
