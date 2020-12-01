// Alex Smirnov 2020-2021


#include "ControllerState.h"

/*void UControllerState::ChangeState(UControllerState& PreviousState)
{

}*/

void UControllerState::SetOtherControllerReference(UControllerState* OtherControllerReference)
{
	OtherController = OtherControllerReference;
}

void UControllerState::SetOwningController(AVirtualRealityMotionController* MotionController)
{
	OwningMotionController = MotionController;
}

uint16 UControllerState::GetControllerStateAsByte()
{
	return CurrentState;
}