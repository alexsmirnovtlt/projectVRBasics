// Alex Smirnov 2020-2021


#include "ControllerState.h"


void UControllerState::SetOtherControllerReference(UControllerState* OtherControllerReference)
{
	OtherController = OtherControllerReference;
}

void UControllerState::SetOwningController(AVirtualRealityMotionController* MotionController)
{
	OwningMotionController = MotionController;
}

void UControllerState::NotifyOtherControllerOfStateChange(bool bStateEntered)
{
	if (OtherController.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("VALID CONTROLLER"));
		OtherController.Get()->OtherControllerStateChanged(this, bStateEntered);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("INVALID CONTROLLER"));
	}
}

uint16 UControllerState::GetControllerStateAsByte() const
{
	return CurrentState;
}