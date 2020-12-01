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
		UE_LOG(LogTemp, Warning, TEXT("StartStateClass was not defined on %s. Empty State was created instead"), *this->GetName());
		ControllerState = NewObject<UControllerState>();
	}

	ControllerState->SetOwningController(this);
}

void AVirtualRealityMotionController::InitialSetup(AVirtualRealityPawn* PawnOwner, FName MotionSource)
{
	OwningPawn = PawnOwner;
	MotionController->SetTrackingMotionSource(MotionSource);

	SetOwner(OwningPawn);
}

void AVirtualRealityMotionController::AddPairedController(AVirtualRealityMotionController* AnotherMotionController)
{
	ControllerState->SetOtherControllerReference(AnotherMotionController->GetControllerState());
}

UControllerState* AVirtualRealityMotionController::GetControllerState()
{
	return ControllerState;
}