// Alex Smirnov 2020-2021


#include "VirtualRealityMotionController.h"

#include "MotionControllerComponent.h"
#include "UObject/ScriptInterface.h"

#include "Interfaces/ControllerPointable.h"
#include "../States/ControllerState.h"
#include "VirtualRealityPawn.h"


AVirtualRealityMotionController::AVirtualRealityMotionController()
{
	PrimaryActorTick.bCanEverTick = true;

	PointingRaycastProfileName = TEXT("BlockAll");
	PointingMaxDistance = 1000.f;

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

void AVirtualRealityMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (ControllerState) ControllerState->Tick(DeltaTime);

	UpdateActorThatItPointsTo(); // Doing a Raycast to determine if we have an object we can interact with (f.e forward input to world placed UI or grabbable objects)
}

void AVirtualRealityMotionController::Destroyed()
{
	Super::Destroyed();

	if (PointedAtActorWithPointableInterface.IsValid()) // TODO Check if that is enough
	{
		IControllerPointable::Execute_OnEndPointed(PointedAtActorWithPointableInterface.Get(), this);
		PointedAtActorWithPointableInterface.Reset();
	}
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

FTransform AVirtualRealityMotionController::GetPointingWorldTransform_Implementation() const
{
	return GetControllerWorldOriginTransform();
}

AVirtualRealityPawn* AVirtualRealityMotionController::GetVRPawn() const
{
	return OwningVRPawn;
}

bool AVirtualRealityMotionController::IsRightHandController()
{
	return IsRightController;
}

AActor* AVirtualRealityMotionController::GetActorToForwardInputTo()
{
	if (bPointedAtActorImplementsInputInterface) return PointedAtActorWithPointableInterface.Get();
	else return nullptr;
}

void AVirtualRealityMotionController::UpdateActorThatItPointsTo()
{
	if (CanDoPointingChecks())
	{
		FHitResult HitResult;
		FVector EndTraceLocation = GetPointingWorldTransform().GetLocation() + GetPointingWorldTransform().GetRotation().Vector() * PointingMaxDistance;

		bool TraceHit = GetWorld()->LineTraceSingleByProfile(HitResult, GetPointingWorldTransform().GetLocation(), EndTraceLocation, PointingRaycastProfileName);
		if (TraceHit && HitResult.Actor.IsValid() && HitResult.Actor.Get()->Implements<UControllerPointable>())
		{
			// Have A valid Hit

			// Notifying previous actor the we ended pointing at it
			if (PointedAtActorWithPointableInterface.IsValid() && PointedAtActorWithPointableInterface.Get() != HitResult.Actor.Get())
			{
				IControllerPointable::Execute_OnEndPointed(PointedAtActorWithPointableInterface.Get(), this);
				
				bPointedAtActorImplementsInputInterface = HitResult.Actor.Get()->Implements<UVRPlayerInput>();
			}

			if (!PointedAtActorWithPointableInterface.IsValid()) bPointedAtActorImplementsInputInterface = HitResult.Actor.Get()->Implements<UVRPlayerInput>();

			PointedAtActorWithPointableInterface = HitResult.Actor;

			USceneComponent* HitComponent = HitResult.Component.IsValid() ? HitResult.Component.Get() : nullptr;

			IControllerPointable::Execute_OnGetPointed(HitResult.Actor.Get(), this, HitComponent, HitResult.Location);
		}
		else if (PointedAtActorWithPointableInterface.IsValid())
		{
			// No Hit
			IControllerPointable::Execute_OnEndPointed(PointedAtActorWithPointableInterface.Get(), this);
			PointedAtActorWithPointableInterface.Reset();
		}
	}
	else if (PointedAtActorWithPointableInterface.IsValid())
	{
		IControllerPointable::Execute_OnEndPointed(PointedAtActorWithPointableInterface.Get(), this);
		PointedAtActorWithPointableInterface.Reset();
	}
}

// Input from Pawn. See VirtualRealityPawn.h for more details

void AVirtualRealityMotionController::PawnInput_Axis_Thumbstick_X(float Value)
{
	if (Value == Axis_Thumbstick_X_Value) return; // To reduce calls so 0, 0 and others wont trigger events continiously
	Axis_Thumbstick_X_Value = Value; // storing value for use in BP
	
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Axis_Thumbstick(ActorToForwardInputTo, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // Forwarding input to some connected actor first 

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if(!ConsumeInputParams.Axes.Thumbstick) if (ControllerState) ControllerState->Execute_Input_Axis_Thumbstick(ControllerState, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // Forwarding input to controller state if input was not consumed by another actor
	}
	else if (ControllerState) ControllerState->Execute_Input_Axis_Thumbstick(ControllerState, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // Forwarding input to controller state if ControllerState is valid

	Execute_Input_Axis_Thumbstick(this, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value); // call to BP event
}

void AVirtualRealityMotionController::PawnInput_Axis_Thumbstick_Y(float Value)
{
	if (Value == Axis_Thumbstick_Y_Value) return;
	Axis_Thumbstick_Y_Value = Value;
	
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Axis_Thumbstick(ActorToForwardInputTo, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Axes.Thumbstick) if (ControllerState) ControllerState->Execute_Input_Axis_Thumbstick(ControllerState, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);
	}
	else if (ControllerState) ControllerState->Execute_Input_Axis_Thumbstick(ControllerState, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);

	Execute_Input_Axis_Thumbstick(this, Axis_Thumbstick_X_Value, Axis_Thumbstick_Y_Value);
}

void AVirtualRealityMotionController::PawnInput_Axis_Trigger(float Value)
{
	if (Value == Axis_Trigger_Value) return;
	Axis_Trigger_Value = Value;

	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Axis_Trigger(ActorToForwardInputTo, Axis_Trigger_Value);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Axes.Trigger) if (ControllerState) ControllerState->Execute_Input_Axis_Trigger(ControllerState, Axis_Trigger_Value);
	}
	else if (ControllerState) ControllerState->Execute_Input_Axis_Trigger(ControllerState, Axis_Trigger_Value);

	Execute_Input_Axis_Trigger(this, Axis_Trigger_Value);
}

void AVirtualRealityMotionController::PawnInput_Axis_Grip(float Value)
{
	if (Value == Axis_Grip_Value) return;
	Axis_Grip_Value = Value;

	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Axis_Grip(ActorToForwardInputTo, Axis_Grip_Value);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Axes.Grip) if (ControllerState) ControllerState->Execute_Input_Axis_Grip(ControllerState, Axis_Grip_Value);
	}
	else if (ControllerState) ControllerState->Execute_Input_Axis_Grip(ControllerState, Axis_Grip_Value);

	Execute_Input_Axis_Grip(this, Axis_Grip_Value);
}

void AVirtualRealityMotionController::PawnInput_Button_Primary(EButtonActionType ActionType)
{
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Button_Primary(ActorToForwardInputTo, ActionType);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Buttons.Primary) if (ControllerState) ControllerState->Execute_Input_Button_Primary(ControllerState, ActionType);
	}
	else if (ControllerState) ControllerState->Execute_Input_Button_Primary(ControllerState, ActionType);

	Execute_Input_Button_Primary(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Secondary(EButtonActionType ActionType)
{
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Button_Secondary(ActorToForwardInputTo, ActionType);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Buttons.Secondary) if (ControllerState) ControllerState->Execute_Input_Button_Secondary(ControllerState, ActionType);
	}
	else if (ControllerState) ControllerState->Execute_Input_Button_Secondary(ControllerState, ActionType);

	Execute_Input_Button_Secondary(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Thumbstick(EButtonActionType ActionType)
{
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Button_Thumbstick(ActorToForwardInputTo, ActionType);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Buttons.Thumbstick) if (ControllerState) ControllerState->Execute_Input_Button_Thumbstick(ControllerState, ActionType);
	}
	else if (ControllerState) ControllerState->Execute_Input_Button_Thumbstick(ControllerState, ActionType);

	Execute_Input_Button_Thumbstick(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Trigger(EButtonActionType ActionType)
{
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Button_Trigger(ActorToForwardInputTo, ActionType);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Buttons.Trigger) if (ControllerState) ControllerState->Execute_Input_Button_Trigger(ControllerState, ActionType);
	}
	else if (ControllerState) ControllerState->Execute_Input_Button_Trigger(ControllerState, ActionType);

	Execute_Input_Button_Trigger(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Grip(EButtonActionType ActionType)
{
	AActor* ActorToForwardInputTo = GetActorToForwardInputTo();
	if (ActorToForwardInputTo)
	{
		IVRPlayerInput::Execute_Input_Button_Grip(ActorToForwardInputTo, ActionType);

		FConsumeInputParams ConsumeInputParams = IVRPlayerInput::Execute_GetConsumeInputParams(ActorToForwardInputTo);
		if (!ConsumeInputParams.Buttons.Grip) if (ControllerState) ControllerState->Execute_Input_Button_Grip(ControllerState, ActionType);
	}
	else if (ControllerState) ControllerState->Execute_Input_Button_Grip(ControllerState, ActionType);

	Execute_Input_Button_Grip(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_Menu(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_Menu(ControllerState, ActionType);
	Execute_Input_Button_Menu(this, ActionType);
}

void AVirtualRealityMotionController::PawnInput_Button_System(EButtonActionType ActionType)
{
	if (ControllerState) ControllerState->Execute_Input_Button_System(ControllerState, ActionType);
	Execute_Input_Button_System(this, ActionType);
}