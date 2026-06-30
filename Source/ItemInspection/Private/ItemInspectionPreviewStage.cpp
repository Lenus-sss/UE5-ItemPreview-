#include "ItemInspectionPreviewStage.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "ItemInspectionConfigComponent.h"
#include "TextureResource.h"

AItemInspectionPreviewStage::AItemInspectionPreviewStage()
{
	// 预览舞台不需要 Tick，拖拽和缩放由 Widget 主动调用。
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PreviewRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot"));
	PreviewRoot->SetupAttachment(SceneRoot);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(SceneRoot);
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->CaptureSource = SCS_SceneColorHDR;
	SceneCapture->FOVAngle = 35.0f;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetVolumetricFog(false);

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(SceneRoot);
	KeyLight->SetIntensity(PreviewLightIntensity);
	KeyLight->SetAttenuationRadius(1200.0f);
	KeyLight->SetRelativeLocation(FVector(120.0f, -160.0f, 180.0f));

	DirectionalFillLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DirectionalFillLight"));
	DirectionalFillLight->SetupAttachment(SceneRoot);
	DirectionalFillLight->SetIntensity(2.5f);
	DirectionalFillLight->SetRelativeRotation(FRotator(-35.0f, 135.0f, 0.0f));

	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(SceneRoot);
	FillLight->SetIntensity(PreviewLightIntensity * 0.35f);
	FillLight->SetAttenuationRadius(1200.0f);
	FillLight->SetRelativeLocation(FVector(-180.0f, 160.0f, 120.0f));
}

bool AItemInspectionPreviewStage::BuildPreviewFromActor(AActor* SourceActor)
{
	// 源 Actor 无效时无法构建预览。
	if (!SourceActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionPreviewStage: SourceActor 为空。"));
		return false;
	}

	ClearPreviewComponents();
	ApplyInspectionConfig(SourceActor);

	const int32 StaticMeshCount = CopyStaticMeshComponents(SourceActor);
	const int32 SkeletalMeshCount = CopySkeletalMeshComponents(SourceActor);
	const int32 TotalMeshCount = StaticMeshCount + SkeletalMeshCount;
	UE_LOG(LogTemp, Log, TEXT("ItemInspectionPreviewStage: 复制 Static=%d Skeletal=%d Total=%d。"), StaticMeshCount, SkeletalMeshCount, TotalMeshCount);

	if (TotalMeshCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionPreviewStage: 没有复制到任何可预览网格。"));
		return false;
	}

	const int32 TargetWidth = FMath::Max(RenderTargetSize.X, 64);
	const int32 TargetHeight = FMath::Max(RenderTargetSize.Y, 64);
	PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("ItemInspectionStageRenderTarget"));
	// SCS_SceneColorHDR 的 A 通道是反透明度：背景清成 1，后续会转成正常 alpha=0。
	PreviewRenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	PreviewRenderTarget->RenderTargetFormat = RTF_RGBA8;
	PreviewRenderTarget->TargetGamma = 2.2f;
	PreviewRenderTarget->InitCustomFormat(TargetWidth, TargetHeight, PF_B8G8R8A8, false);
	PreviewRenderTarget->UpdateResourceImmediate(true);
	CreateOrResizeDisplayTexture(TargetWidth, TargetHeight);

	SceneCapture->TextureTarget = PreviewRenderTarget;
	SceneCapture->ClearShowOnlyComponents();
	for (UPrimitiveComponent* PreviewComponent : PreviewComponents)
	{
		SceneCapture->ShowOnlyComponent(PreviewComponent);
	}

	RefreshPreviewBounds();
	UpdateCaptureCamera();
	CapturePreview();

	return true;
}

void AItemInspectionPreviewStage::AddOrbitRotation(const FVector2D& MouseDelta, float DragRotationSpeed)
{
	// 鼠标拖拽方向取反，让物体视觉上跟随鼠标移动方向旋转。
	PreviewRoot->AddLocalRotation(FRotator(0.0f, -MouseDelta.X * DragRotationSpeed, 0.0f));
	CameraPitch = FMath::Clamp(CameraPitch - MouseDelta.Y * DragRotationSpeed, -80.0f, 80.0f);
	UpdateCaptureCamera();
	CapturePreview();
}

void AItemInspectionPreviewStage::AddCameraZoom(float WheelDelta, float WheelZoomSpeed)
{
	// 滚轮向上拉近，向下拉远。
	const float ZoomScale = FMath::Clamp(1.0f - WheelDelta * WheelZoomSpeed, 0.2f, 5.0f);
	CameraDistance = FMath::Clamp(CameraDistance * ZoomScale, MinCameraDistance, MaxCameraDistance);
	UpdateCaptureCamera();
	CapturePreview();
}

void AItemInspectionPreviewStage::CapturePreview()
{
	// 手动捕获避免每帧自动刷新带来的额外开销和警告。
	if (SceneCapture && PreviewRenderTarget)
	{
		SceneCapture->CaptureScene();
		UpdateDisplayTextureFromRenderTarget();
	}
}

void AItemInspectionPreviewStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 结束时释放运行时创建的 RenderTarget 引用。
	PreviewRenderTarget = nullptr;
	PreviewDisplayTexture = nullptr;
	PreviewComponents.Reset();
	Super::EndPlay(EndPlayReason);
}

void AItemInspectionPreviewStage::ClearPreviewComponents()
{
	// 清理上一次复制出的组件，避免重复打开时叠模型。
	for (UPrimitiveComponent* PreviewComponent : PreviewComponents)
	{
		if (PreviewComponent)
		{
			PreviewComponent->DestroyComponent();
		}
	}

	PreviewComponents.Reset();
	SceneCapture->ClearShowOnlyComponents();
	PreviewDisplayTexture = nullptr;
}

int32 AItemInspectionPreviewStage::CopyStaticMeshComponents(AActor* SourceActor)
{
	// 复制 StaticMesh、材质和相对变换。
	TArray<UStaticMeshComponent*> SourceComponents;
	SourceActor->GetComponents(SourceComponents);

	int32 CopiedCount = 0;
	for (UStaticMeshComponent* SourceComponent : SourceComponents)
	{
		if (!ShouldCopyMeshComponent(SourceComponent) || !SourceComponent->GetStaticMesh())
		{
			continue;
		}

		UStaticMeshComponent* PreviewComponent = NewObject<UStaticMeshComponent>(this);
		PreviewComponent->SetMobility(EComponentMobility::Movable);
		PreviewComponent->SetStaticMesh(SourceComponent->GetStaticMesh());
		PreviewComponent->SetRelativeTransform(SourceComponent->GetComponentTransform().GetRelativeTransform(SourceActor->GetActorTransform()));
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetCastShadow(false);

		for (int32 MaterialIndex = 0; MaterialIndex < SourceComponent->GetNumMaterials(); ++MaterialIndex)
		{
			PreviewComponent->SetMaterial(MaterialIndex, SourceComponent->GetMaterial(MaterialIndex));
		}

		AddInstanceComponent(PreviewComponent);
		PreviewComponent->AttachToComponent(PreviewRoot, FAttachmentTransformRules::KeepRelativeTransform);
		PreviewComponent->RegisterComponent();
		PreviewComponents.Add(PreviewComponent);
		++CopiedCount;
	}

	return CopiedCount;
}

int32 AItemInspectionPreviewStage::CopySkeletalMeshComponents(AActor* SourceActor)
{
	// 复制 SkeletalMesh 和材质；动画状态不复制。
	TArray<USkeletalMeshComponent*> SourceComponents;
	SourceActor->GetComponents(SourceComponents);

	int32 CopiedCount = 0;
	for (USkeletalMeshComponent* SourceComponent : SourceComponents)
	{
		if (!ShouldCopyMeshComponent(SourceComponent) || !SourceComponent->GetSkeletalMeshAsset())
		{
			continue;
		}

		USkeletalMeshComponent* PreviewComponent = NewObject<USkeletalMeshComponent>(this);
		PreviewComponent->SetMobility(EComponentMobility::Movable);
		PreviewComponent->SetSkeletalMeshAsset(SourceComponent->GetSkeletalMeshAsset());
		PreviewComponent->SetRelativeTransform(SourceComponent->GetComponentTransform().GetRelativeTransform(SourceActor->GetActorTransform()));
		PreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PreviewComponent->SetCastShadow(false);

		for (int32 MaterialIndex = 0; MaterialIndex < SourceComponent->GetNumMaterials(); ++MaterialIndex)
		{
			PreviewComponent->SetMaterial(MaterialIndex, SourceComponent->GetMaterial(MaterialIndex));
		}

		AddInstanceComponent(PreviewComponent);
		PreviewComponent->AttachToComponent(PreviewRoot, FAttachmentTransformRules::KeepRelativeTransform);
		PreviewComponent->RegisterComponent();
		PreviewComponents.Add(PreviewComponent);
		++CopiedCount;
	}

	return CopiedCount;
}

void AItemInspectionPreviewStage::RefreshPreviewBounds()
{
	// 用所有复制组件的包围盒计算相机目标点。
	FBox PreviewBox(ForceInit);
	for (UPrimitiveComponent* PreviewComponent : PreviewComponents)
	{
		if (PreviewComponent)
		{
			PreviewBox += PreviewComponent->Bounds.GetBox();
		}
	}

	if (!PreviewBox.IsValid)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionPreviewStage: 预览包围盒无效，使用默认相机参数。"));
		PreviewCenter = GetActorLocation();
		PreviewRadius = 50.0f;
		CameraDistance = 120.0f;
		return;
	}

	PreviewCenter = PreviewBox.GetCenter();
	PreviewRadius = FMath::Max(PreviewBox.GetExtent().Size(), 20.0f);
	const float HalfFOVRadians = FMath::DegreesToRadians(FMath::Max(SceneCapture->FOVAngle, 1.0f) * 0.5f);
	const float FitDistance = PreviewRadius / FMath::Max(FMath::Tan(HalfFOVRadians), 0.01f);
	CameraDistance = FMath::Clamp(FitDistance * CameraDistanceScale + CameraDistanceOffset, MinCameraDistance, MaxCameraDistance);
	UpdatePreviewLighting();
	UE_LOG(LogTemp, Log, TEXT("ItemInspectionPreviewStage: Bounds Center=%s Extent=%s Radius=%.2f CameraDistance=%.2f Scale=%.2f Offset=%.2f。"),
		*PreviewCenter.ToString(),
		*PreviewBox.GetExtent().ToString(),
		PreviewRadius,
		CameraDistance,
		CameraDistanceScale,
		CameraDistanceOffset);
}

void AItemInspectionPreviewStage::UpdateCaptureCamera()
{
	// 相机围绕模型中心拍摄，始终看向包围盒中心。
	const FRotator OrbitRotation(CameraPitch, CameraYaw, 0.0f);
	const FVector CameraDirection = OrbitRotation.Vector();
	const FVector CameraLocation = PreviewCenter - CameraDirection * CameraDistance;
	const FRotator LookAtRotation = (PreviewCenter - CameraLocation).Rotation();

	SceneCapture->SetWorldLocationAndRotation(CameraLocation, LookAtRotation);
	UpdatePreviewLighting();
}

bool AItemInspectionPreviewStage::ShouldCopyMeshComponent(const UPrimitiveComponent* Component) const
{
	// 默认跳过隐藏组件，避免把辅助模型复制进预览。
	if (!Component)
	{
		return false;
	}

	return !bOnlyCopyVisibleMeshComponents || Component->IsVisible();
}

void AItemInspectionPreviewStage::ApplyInspectionConfig(const AActor* SourceActor)
{
	// 每次构建预览前恢复默认值，避免上一个 Actor 的配置影响下一个 Actor。
	CameraDistanceScale = DefaultCameraDistanceScale;
	CameraDistanceOffset = 0.0f;
	SceneCapture->FOVAngle = 35.0f;
	MinCameraDistance = 35.0f;
	MaxCameraDistance = 3000.0f;

	const UItemInspectionConfigComponent* ConfigComponent = SourceActor ? SourceActor->FindComponentByClass<UItemInspectionConfigComponent>() : nullptr;
	if (!ConfigComponent)
	{
		return;
	}

	CameraDistanceScale = FMath::Max(ConfigComponent->CameraDistanceScale, 0.1f);
	CameraDistanceOffset = ConfigComponent->CameraDistanceOffset;

	if (ConfigComponent->CameraFOVOverride > 0.0f)
	{
		SceneCapture->FOVAngle = FMath::Clamp(ConfigComponent->CameraFOVOverride, 5.0f, 170.0f);
	}

	if (ConfigComponent->MinCameraDistanceOverride > 0.0f)
	{
		MinCameraDistance = ConfigComponent->MinCameraDistanceOverride;
	}

	if (ConfigComponent->MaxCameraDistanceOverride > 0.0f)
	{
		MaxCameraDistance = FMath::Max(ConfigComponent->MaxCameraDistanceOverride, MinCameraDistance);
	}
}

void AItemInspectionPreviewStage::UpdatePreviewLighting()
{
	// 灯光跟随包围盒中心和半径，避免大模型只有局部被照亮。
	const float LightDistance = FMath::Max(PreviewRadius * 3.0f, 180.0f);
	const float AttenuationRadius = FMath::Max(PreviewRadius * 6.0f, 1200.0f);
	const float RadiusScale = FMath::Max(PreviewRadius / 50.0f, 1.0f);

	KeyLight->SetWorldLocation(PreviewCenter + FVector(LightDistance, -LightDistance, LightDistance * 0.9f));
	KeyLight->SetIntensity(PreviewLightIntensity * RadiusScale);
	KeyLight->SetAttenuationRadius(AttenuationRadius);

	FillLight->SetWorldLocation(PreviewCenter + FVector(-LightDistance * 0.8f, LightDistance, LightDistance * 0.55f));
	FillLight->SetIntensity(PreviewLightIntensity * 0.45f * RadiusScale);
	FillLight->SetAttenuationRadius(AttenuationRadius);

	DirectionalFillLight->SetWorldRotation(FRotator(-35.0f, 135.0f, 0.0f));
}

void AItemInspectionPreviewStage::CreateOrResizeDisplayTexture(int32 Width, int32 Height)
{
	if (PreviewDisplayTexture && PreviewDisplayTexture->GetSizeX() == Width && PreviewDisplayTexture->GetSizeY() == Height)
	{
		return;
	}

	PreviewDisplayTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, TEXT("ItemInspectionPreviewDisplayTexture"));
	if (PreviewDisplayTexture)
	{
		PreviewDisplayTexture->SRGB = true;
		PreviewDisplayTexture->NeverStream = true;
		PreviewDisplayTexture->LODGroup = TEXTUREGROUP_UI;
		PreviewDisplayTexture->UpdateResource();
	}
}

void AItemInspectionPreviewStage::UpdateDisplayTextureFromRenderTarget()
{
	if (!PreviewRenderTarget)
	{
		return;
	}

	const int32 Width = PreviewRenderTarget->SizeX;
	const int32 Height = PreviewRenderTarget->SizeY;
	CreateOrResizeDisplayTexture(Width, Height);

	if (!PreviewDisplayTexture || !PreviewDisplayTexture->GetPlatformData() || PreviewDisplayTexture->GetPlatformData()->Mips.Num() <= 0)
	{
		return;
	}

	FTextureRenderTargetResource* RenderTargetResource = PreviewRenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		return;
	}

	TArray<FColor> CapturedPixels;
	if (!RenderTargetResource->ReadPixels(CapturedPixels) || CapturedPixels.Num() != Width * Height)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspectionPreviewStage: 读取 RenderTarget 像素失败，无法更新透明显示贴图。"));
		return;
	}

	for (FColor& Pixel : CapturedPixels)
	{
		// UE 的 SceneColorHDR alpha 是反透明度：0 表示不透明，255 表示全透明。
		Pixel.A = 255 - Pixel.A;
		if (Pixel.A == 0)
		{
			Pixel.R = 0;
			Pixel.G = 0;
			Pixel.B = 0;
		}
	}

	FTexture2DMipMap& Mip = PreviewDisplayTexture->GetPlatformData()->Mips[0];
	void* TextureData = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, CapturedPixels.GetData(), CapturedPixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	PreviewDisplayTexture->UpdateResource();
}
