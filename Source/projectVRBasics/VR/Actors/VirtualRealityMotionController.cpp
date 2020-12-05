// Alex Smirnov 2020-2021


#include "VirtualRealityMotionController.h"

#include "MotionControllerComponent.h"
#include "VirtualRealityPawn.h"

#include "../States/ControllerState.h"


AVirtualRealityMotionController::AVirtualRealityMotionController()
{
	PrimaryActorTick.bCanEverTick = true;

	auto NewRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = NewRootComponent;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerComponent"));
	MotionController->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	// TODO Check if it may lead to potential problems (multiplayer?)
	AutoReceiveInput = EAutoReceiveInput::Player0; // Input is defined in BP child classes, so every controller can define custom visual and logic behaviour (thumbstick rotation, button presses etc)
}

void AVirtualRealityMotionController::BeginPlay()
{
	Super::BeginPlay();

	ChangeToDefaultState(false);
}

void AVirtualRealityMotionController::InitialSetup(AVirtualRealityPawn* PawnOwner, FName MotionSource)
{
	OwningVRPawn = PawnOwner;
	MotionController->SetTrackingMotionSource(MotionSource);

	SetOwner(OwningVRPawn);
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
			//trollerState->SetOtherControllerReference();
		}

		if (NotifyPairedControllerIfAble)
		{
			ControllerState->NotifyPairedControllerOfStateChange(true);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Class %s was not defined on %s. Empty State was created instead. Input will not work"), *NewStateClass.Get()->GetClass()->GetName(), *this->GetName());
		ControllerState = NewObject<UControllerState>();
	}
}

void AVirtualRealityMotionController::ChangeToDefaultState(bool NotifyPairedControllerIfAble)
{
	ChangeState(StartStateClass, NotifyPairedControllerIfAble);
}

UControllerState* AVirtualRealityMotionController::GetControllerState() const
{
	return ControllerState;
}

FVector AVirtualRealityMotionController::GetControllerWorldOriginLocation_Implementation() const
{
	return MotionController->GetComponentLocation();
}

FRotator AVirtualRealityMotionController::GetControllerWorldOriginRotation_Implementation() const
{
	return MotionController->GetComponentRotation();
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