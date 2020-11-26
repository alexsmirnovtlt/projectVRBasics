// Alex Smirnov 2020-2021


#include "VirtualRealityMotionController.h"

#include "MotionControllerComponent.h"


AVirtualRealityMotionController::AVirtualRealityMotionController()
{
	PrimaryActorTick.bCanEverTick = true;

	auto NewRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = NewRootComponent;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionControllerComponent"));
	MotionController->AttachTo(RootComponent);
}

void AVirtualRealityMotionController::BeginPlay()
{
	Super::BeginPlay();
	//MotionController.model
	//MotionController.GetSystem
	//UE_LOG(LogTemp, Warning, TEXT("DisplayModelSource: %s"), *MotionController->DisplayModelSource.ToString());
	
	//UE_LOG(LogTemp, Warning, TEXT("DisplayModelSource: %s"), *MotionController->CustomModelSourceId.ToString());
	//UE_LOG(LogTemp, Warning, TEXT("MotionSource: %s"), *MotionController->CustomDisplayMesh->GetName());
}

void AVirtualRealityMotionController::InitialSetup(FName MotionSource)
{
	MotionController->SetTrackingMotionSource(MotionSource);
}

void AVirtualRealityMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

