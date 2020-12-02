// Alex Smirnov 2020-2021


#include "ControllerState.h"


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

int32 UControllerState::GetControllerStateAsInt_Implementation() const
{
	return 0; // Must be overridden in BP because State Enum was created in the Editor so when new state is added, project C++ code should`nt be rebuild
}