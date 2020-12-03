// Alex Smirnov 2020-2021


#include "VRTeleportLogic.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h"

#include "../../../General/GameInstances/GameInstWithStreamableManager.h"


void UVRTeleportLogic::Initialize()
{
	auto GameInstance = UGameplayStatics::GetGameInstance(this);
	if (!GameInstance)
	{
		return;
	}

	auto GameInstanceWithStreamableManager = Cast<UGameInstWithStreamableManager>(GameInstance);
	if (!GameInstanceWithStreamableManager)
	{
		return;
	}
	
	FStreamableManager& StreamableManager = GameInstanceWithStreamableManager->GetStreamableManager();
	
	TeleportArrowHandle = StreamableManager.RequestAsyncLoad(TeleportArrowClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UVRTeleportLogic::OnAssetLoaded));
	TeleportBeamHandle = StreamableManager.RequestAsyncLoad(TeleportBeamPartClass.ToSoftObjectPath(), FStreamableDelegate::CreateUObject(this, &UVRTeleportLogic::OnAssetLoaded));
}

void UVRTeleportLogic::HandleDestruction()
{
	// TODO Check if thats enough

	if (TeleportArrowHandle.IsValid())
	{
		TeleportArrowHandle.Get()->ReleaseHandle();
	}
	if (TeleportBeamHandle.IsValid())
	{
		TeleportBeamHandle.Get()->ReleaseHandle();
	}

	if (TeleportArrowActor)
	{
		TeleportArrowActor->Destroy();
	}
	if (TeleportBeamPartActor)
	{
		TeleportBeamPartActor->Destroy();
	}
}

void UVRTeleportLogic::OnAssetLoaded()
{
	if (!TeleportArrowHandle.IsValid() || !TeleportBeamHandle.IsValid())
	{
		return;
	}

	if (TeleportArrowHandle.Get()->HasLoadCompleted() && !TeleportArrowActor)
	{
		auto TeleportArrowLoadedClass = Cast<UClass>(TeleportArrowHandle.Get()->GetLoadedAsset());
		TeleportArrowActor = GetWorld()->SpawnActor<AActor>(TeleportArrowLoadedClass);

		if (TeleportArrowActor)
		{
			TeleportArrowActor->SetActorHiddenInGame(false);
		}
	}

	if (TeleportBeamHandle.Get()->HasLoadCompleted() && !TeleportBeamPartActor)
	{
		auto TeleportTeleportBeamLoadedClass = Cast<UClass>(TeleportBeamHandle.Get()->GetLoadedAsset());
		TeleportBeamPartActor = GetWorld()->SpawnActor(TeleportTeleportBeamLoadedClass);

		if (TeleportBeamPartActor)
		{
			TeleportBeamPartActor->SetActorHiddenInGame(false);
		}
	}
}

AActor* UVRTeleportLogic::GetArrowActor()
{
	return TeleportArrowActor;
}

AActor* UVRTeleportLogic::GetTeleportBeamActor()
{
	return TeleportBeamPartActor;
}