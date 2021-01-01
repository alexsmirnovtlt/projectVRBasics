// Alex Smirnov 2020-2021


#include "VirtualRealityMotionController.h"


#include "MotionControllerComponent.h"
#include "UObject/ScriptInterface.h"

#include "../States/ControllerState.h"
#include "VirtualRealityPawn.h"


AVirtualRealityMotionController::AVirtualRealityMotionController()
{
	PrimaryActorTick.bCanEverTick = true;

	auto NewRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = NewRootComponent;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerComponent"));
	MotionController->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	MotionController->SetGenerateOverlapEvents(false);
	MotionController->SetCollisionProfileName("NoCollision");
}

void AVirtualRealityMotionController::BeginPlay()
{
	Super::BeginPlay();

	ChangeToDefaultState(false);
}

void AVirtualRealityMotionController::InitialSetup(AVirtualRealityPawn* PawnOwner, bool IsLeft, bool IsPrimary)
{
	OwningVRPawn = PawnOwner;
	IsRightController = !IsLeft;

	FName HandName = IsRightController ? TEXT("Right") : TEXT("Left");
	MotionController->SetTrackingMotionSource(HandName);
	
	IsControllerPrimary = IsPrimary;

	SetOwner(OwningVRPawn);
	
	OnDoneInitByPawn();
}

void AVirtualRealityMotionController::PairControllers(AVirtualRealityMotionController* AnotherMotionController)
{
	ControllerState->SetOtherControllerReference(AnotherMotionController->GetControllerState());

	auto AnotherControllerState = AnotherMotionController->GetControllerState();
	if (AnotherControllerState)
	{
		AnotherControllerState->SetOtherControllerReference(ControllerState);
	}
}

void AVirtualRealityMotionController::ChangeState(TSubclassOf<UControllerState> NewStateClass, bool NotifyPairedControllerIfAble)
{
	// Switching controller state, exiting current state, deleting that object and creating new one.
	// Example: We started with idle state, no input from the player. Then Player moves left stick up. We are switching state from idle to Teleport_Find. Possible input bindings are changed in this state (f.e we will be unable to grab items in this state or explicitly state that teleport will be aborted in that case)

	UControllerState* PairedControllerReference = nullptr;

	if (ControllerState)
	{
		ControllerState->OnStateExit();
		PairedControllerReference = ControllerState->GetPairedControllerState(); // we got to keep references between controllers if able. Caching then applying it back to a new state
	
		if (NotifyPairedControllerIfAble)
		{
			ControllerState->NotifyPairedControllerOfStateChange(false);
		}
	}

	// Dont need to explicitly delete current ControllerState because there will be no references to it and it will be garbage collected (Parent class is UObject, not AActor)

	if (NewStateClass)
	{
		ControllerState = NewObject<UControllerState>(this, NewStateClass);
		ControllerState->SetOwningController(this);
		ControllerState->OnStateEnter();

		if (PairedControllerReference)
		{
			PairControllers(PairedControllerReference->GetOwningMotionController());
		}

		if (NotifyPairedControllerIfAble)
		{
			ControllerState->NotifyPairedControllerOfStateChange(true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Trying to create undefined state on %s. Empty State was created instead. Input will not work"), *this->GetName());
		ControllerState = NewObject<UControllerState>();
	}
}

void AVirtualRealityMotionController::ChangeToDefaultState(bool NotifyPairedControllerIfAble)
{
	ChangeState(StartStateClass, NotifyPairedControllerIfAble);
}

void AVirtualRealityMotionController::OnPawnTeleport(bool bStarted, bool bCameraViewOnly)
{
	OnPawnTeleportEvent(bStarted, bCameraViewOnly);
}

UControllerState* AVirtualRealityMotionController::GetControllerState() const
{
	return ControllerState;
}

FTransform AVirtualRealityMotionController::GetControllerWorldOriginTransform_Implementation() const
{
	return MotionController->GetComponentTransform();
}

USplineComponent* AVirtualRealityMotionController::GetSplineComponent_Implementation() const
{
	UE_LOG(LogTemp, Error, TEXT("Blueprint \"%s\" must override function GetSplineComponent()"), *this->GetClass()->GetFName().ToString());
	return nullptr;
}

AVirtualRealityPawn* AVirtualRealityMotionController::GetVRPawn() const
{
	return OwningVRPawn;
}

bool AVirtualRealityMotionController::IsRightHandController()
{
	return IsRightController;
}

// Input from Pawn. See VirtualRealityPawn.h for more details

void AVirtualRealityMotionController::PawnInput_Axis_Thumbstick_X(float Value)
{
	Axis_Thumbstick_X_Value = Value; // storing value for use in BP
	if (ControllerState) ControllerState->Execute_Input_Axis_Thumbstick(ControllerState, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // Forwarding input to controller state if able
	 // TODO MUST CHECK BELOW
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Axis_Thumbstick(ConnectedActorWithInputInterface.GetObject(), Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // Forwarding to grabbed object or UI currently in use
	Execute_Input_Axis_Thumbstick(this, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // call to BP event
}

void AVirtualRealityMotionController::PawnInput_Axis_Thumbstick_Y(float Value)
{
	Axis_Thumbstick_Y_Value = Value;
	if (ControllerState) ControllerState->Execute_Input_Axis_Thumbstick(ControllerState, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Axis_Thumbstick(ConnectedActorWithInputInterface.GetObject(), Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);
	Execute_Input_Axis_Thumbstick(this, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);
}

void AVirtualRealityMotionController::PawnInput_Axis_Trigger(float Value)
{
	Axis_Trigger_Value = Value;
	if (ControllerState) ControllerState->Execute_Input_Axis_Trigger(ControllerState, Value);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Axis_Trigger(ConnectedActorWithInputInterface.GetObject(), Value);
	Execute_Input_Axis_Trigger(this, Value);
}

void AVirtualRealityMotionController::PawnInput_Axis_Grip(float Value)
{
	Axis_Grip_Value = Value;
	if (ControllerState) ControllerState->Execute_Input_Axis_Grip(ControllerState, Value);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Axis_Grip(ConnectedActorWithInputInterface.GetObject(), Value);
	Execute_Input_Axis_Grip(this, Value);
}

void AVirtualRealityMotionController::PawnInput_Button_Primary(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Primary(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_Primary(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_Primary(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Secondary(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Secondary(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_Secondary(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_Secondary(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Thumbstick(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Thumbstick(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_Thumbstick(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_Thumbstick(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Trigger(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Trigger(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_Trigger(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_Trigger(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Grip(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Grip(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_Grip(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_Grip(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Menu(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Menu(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_Menu(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_Menu(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_System(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_System(ControllerState, ActionType);
	if (ConnectedActorWithInputInterface) ConnectedActorWithInputInterface->Execute_Input_Button_System(ConnectedActorWithInputInterface.GetObject(), ActionType);
	Execute_Input_Button_System(this, ActionType);
}