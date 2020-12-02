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

	if (StartStateClass)
	{
		ControllerState = NewObject<UControllerState>(this, StartStateClass);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("StartStateClass was not defined on %s. Empty State was created instead. Input will not work"), *this->GetName());
		ControllerState = NewObject<UControllerState>();
	}

	ControllerState->SetOwningController(this);
	ControllerState->OnStateEnter();
}

void AVirtualRealityMotionController::InitialSetup(AVirtualRealityPawn* PawnOwner, FName MotionSource)
{
	OwningVRPawn = PawnOwner;
	MotionController->SetTrackingMotionSource(MotionSource);

	SetOwner(OwningVRPawn);
}

void AVirtualRealityMotionController::AddPairedController(AVirtualRealityMotionController* AnotherMotionController)
{
	ControllerState->SetOtherControllerReference(AnotherMotionController->GetControllerState());
}

void AVirtualRealityMotionController::ChangeState(TSubclassOf<UControllerState> NewStateClass, UControllerState* PreviousState)
{
	// Switching controller state, exiting current state, deleting that object and creating new one.
	// Example: We started with idle state, no input from the player. Then Player moves left stick up. We are switching state from idle to Teleport_Find. Possible input bindings are changed in this state (f.e we will be unable to grab items in this state or explicitly state that teleport will be aborted in that case)

	PreviousState->OnStateExit();
	PreviousState->NotifyOtherControllerOfStateChange(false);

	// Dont need to explicitly delete current ControllerState because there will be no references to it and it will be garbage collected (Parent class is UObject, not AActor)

	if (NewStateClass)
	{
		ControllerState = NewObject<UControllerState>(this, NewStateClass);
		ControllerState->OnStateEnter();
		ControllerState->NotifyOtherControllerOfStateChange(true);
	}
}

UControllerState* AVirtualRealityMotionController::GetControllerState()
{
	return ControllerState;
}

FVector AVirtualRealityMotionController::GetControllerWorldOriginLocation_Implementation() const
{
	return GetActorLocation();
}

FRotator AVirtualRealityMotionController::GetControllerWorldOriginRotation_Implementation() const
{
	return GetActorRotation();
}