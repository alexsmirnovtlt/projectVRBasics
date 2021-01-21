// Alex Smirnov 2020-2021


#include "ControllerState.h"

#include "Engine/World.h"

#include "../Actors/VirtualRealityMotionController.h"


void UControllerState::SetOtherControllerReference(UControllerState* OtherControllerReference)
{
	PairedController = OtherControllerReference;
}

void UControllerState::SetOwningController(AVirtualRealityMotionController* MotionController)
{
	OwningMotionController = MotionController;
}

void UControllerState::NotifyPairedControllerOfStateChange(bool bStateEntered)
{
	if (PairedController.IsValid())
	{
		PairedController.Get()->PairedControllerStateChanged(this, bStateEntered);
	}
}

UControllerState* UControllerState::GetPairedControllerState()
{
	if (!PairedController.IsValid())
	{
		return nullptr;
	}

	return PairedController.Get();
}

AVirtualRealityMotionController* UControllerState::GetOwningMotionController() const
{
	return OwningMotionController;
}

uint8 UControllerState::GetControllerStateAsByte_Implementation() const
{
	return 0; // Must be overridden in BP because State Enum was created in the Editor so after new state is added, no need to edit C++ code and recompile
}

AActor* UControllerState::SpawnActor(TSubclassOf<AActor> ClassToSpawn)
{
	if (!GetWorld()) return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnParams);
	SpawnedActor->SetOwner(OwningMotionController);

	return SpawnedActor;
}