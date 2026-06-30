#include "ItemInspectionBlueprintLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "ItemInspectionWidget.h"
#include "Kismet/GameplayStatics.h"

UItemInspectionWidget* UItemInspectionBlueprintLibrary::OpenItemInspection(const UObject* WorldContextObject, AActor* ActorToInspect, const FVector2D& ScreenPosition, TSubclassOf<UItemInspectionWidget> WidgetClass, int32 PlayerIndex, int32 ZOrder)
{
	// WorldContextObject 让蓝图可以从任意 Actor、Component 或 Widget 调用本函数。
	if (!WorldContextObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspection: WorldContextObject 为空，无法创建检视 Widget。"));
		return nullptr;
	}

	// 必须传入一个可检视 Actor 和一个继承 UItemInspectionWidget 的 Widget Blueprint。
	if (!ActorToInspect)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspection: ActorToInspect 为空，无法创建检视 Widget。"));
		return nullptr;
	}

	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspection: WidgetClass 为空，无法创建检视 Widget。"));
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspection: 无法从 WorldContextObject 获取 World。"));
		return nullptr;
	}

	// 使用指定玩家控制器创建 Widget，单机默认 PlayerIndex 为 0。
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspection: 找不到 PlayerIndex=%d 的 PlayerController。"), PlayerIndex);
		return nullptr;
	}

	UItemInspectionWidget* InspectionWidget = CreateWidget<UItemInspectionWidget>(PlayerController, WidgetClass);
	if (!InspectionWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInspection: CreateWidget 失败，WidgetClass=%s。"), *GetNameSafe(WidgetClass));
		return nullptr;
	}

	InspectionWidget->AddToPlayerScreen(ZOrder);
	InspectionWidget->SetupInspection(ActorToInspect, ScreenPosition);
	UE_LOG(LogTemp, Log, TEXT("ItemInspection: 已创建并加入视口，Actor=%s，Widget=%s。"), *GetNameSafe(ActorToInspect), *GetNameSafe(InspectionWidget));

	return InspectionWidget;
}
