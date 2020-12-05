// Alex Smirnov 2020-2021


#include "VirtualRealityPawn.h"

#include "VirtualRealityMotionController.h"
#include "MotionControllerComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "IXRTrackingSystem.h"
#include "IXRSystemAssets.h"
#include "Engine/World.h"

#include "../States/ControllerState.h"


AVirtualRealityPawn::AVirtualRealityPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root
	PawnRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = PawnRootComponent;
	// VR Camera
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	MainCamera->AttachTo(RootComponent);

	// Cached attachment properties for controllers creation
	AttachmentRules.ScaleRule = EAttachmentRule::KeepWorld;
}

void AVirtualRealityPawn::BeginPlay()
{
	Super::BeginPlay();

	TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> TrackingSystem = GEngine->XRSystem;
	if (!ensure(TrackingSystem.Get())) { return; }

	// Check that VR Headset is present and set tracking origin
	if (!InitHeadset(*TrackingSystem)) return;
	// Trying to create motion controlles using StartingControllerName if not none, or detecting Headset type using info from TrackingSystem
	InitMotionControllers(*TrackingSystem);
}

bool AVirtualRealityPawn::InitHeadset(IXRTrackingSystem& TrackingSystem)
{
	uint32 HMDCount = TrackingSystem.CountTrackedDevices(EXRTrackedDeviceType::HeadMountedDisplay);

	if (HMDCount != 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Headset was not detected"));
		UGameplayStatics::OpenLevel(GetWorld()->GetGameInstance(), StartupLevelName);
		return false;
	}
	// Setting that our camera will change its local position relative to the real world floor
	TrackingSystem.SetTrackingOrigin(EHMDTrackingOrigin::Floor); // will work for everything except PS Move, EHMDTrackingOrigin::Eye for that

	return true;
}

void AVirtualRealityPawn::InitMotionControllers(IXRTrackingSystem& TrackingSystem)
{
	bool bSuccessControllersCreation = false;

	if (!StartingControllerName.IsNone())
	{
		bSuccessControllersCreation = SwitchMotionControllers(StartingControllerName);
	}

	if(!bSuccessControllersCreation)
	{
		FString HeadsetInfo = TrackingSystem.GetVersionString();
		for (auto& HeadsetType : ControllerTypes)
		{
			if (HeadsetInfo.Contains(HeadsetType.HeadsetName.ToString()))
			{
				bSuccessControllersCreation = SwitchMotionControllers(HeadsetType.HeadsetName);
			}
		}
	}

	if (!bSuccessControllersCreation)
	{
		UE_LOG(LogTemp, Error, TEXT("Could not find Motion Controller Class to Spawn. Check StartingControllerName and/or ControllerTypes fields at %s"), *this->GetClass()->GetName());
	}
}

bool AVirtualRealityPawn::SwitchMotionControllers(FName NewProfileName)
{
	for (auto& HeadsetType : ControllerTypes)
	{
		if (HeadsetType.HeadsetName.IsEqual(NewProfileName))
		{
			CurrentControllersTypeName = NewProfileName;

			auto LeftHandClass = HeadsetType.LeftController.LoadSynchronous();
			auto RightHandClass = HeadsetType.RightController.LoadSynchronous();
			if (ensure(LeftHandClass && RightHandClass))
			{
				CreateMotionController(true, LeftHandClass);
				CreateMotionController(false, RightHandClass);

				LeftHand->PairControllers(RightHand);
			}

			return true;
		}
	}

	CurrentControllersTypeName = TEXT("None");
	return false;
}

void AVirtualRealityPawn::CreateMotionController(bool bLeft, UClass* ClassToCreate)
{
	if (bLeft)
	{
		if (LeftHand)
		{
			auto ControllerState = LeftHand->GetControllerState();
			if (ControllerState)
			{
				ControllerState->OnStateExit();
			}
			LeftHand->Destroy();
		}

		LeftHand = GetWorld()->SpawnActor<AVirtualRealityMotionController>(ClassToCreate, FVector::ZeroVector, FRotator::ZeroRotator);
		LeftHand->AttachToComponent(RootComponent, AttachmentRules);

		LeftHand->InitialSetup(this, TEXT("Left"));
	}
	else
	{
		if (RightHand)
		{
			auto ControllerState = RightHand->GetControllerState();
			if (ControllerState)
			{
				ControllerState->OnStateExit();
			}
			RightHand->Destroy();
		}

		RightHand = GetWorld()->SpawnActor<AVirtualRealityMotionController>(ClassToCreate, FVector::ZeroVector, FRotator::ZeroRotator);
		RightHand->AttachToComponent(RootComponent, AttachmentRules);
		
		RightHand->InitialSetup(this, TEXT("Right"));
	}
}

void AVirtualRealityPawn::AddCameraYawRotation(float YawToAdd)
{
	// Main problem is we cant just rotate Camera component. So we are making that root component rotates around current camera position so camera may stay in the same place but change its rotation because of a root component 
	// If we just rotate Camera component or root, if we are not standing right at 0,0,0 local position in the real world, we wil be changing position as we rotate

	FVector MainCameraLocationProjected = MainCamera->GetComponentLocation() * FVector(1.f, 1.f, 0.f);
	FVector RootMoveDirection = MainCameraLocationProjected - GetActorLocation();
	FVector RotatedRootMoveDirection = FRotator(0.f, YawToAdd, 0.f).RotateVector(RootMoveDirection);

	SetActorLocation(MainCameraLocationProjected - RotatedRootMoveDirection);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw + YawToAdd, 0.f));
}

void AVirtualRealityPawn::TeleportToLocation(FVector NewLocation, FRotator NewRotation, bool bResetLocalPosition)
{
	if (!bResetLocalPosition)
	{
		// Just Teleports Pawn to NewLocation
		SetActorLocation(NewLocation);
		SetActorRotation(NewRotation);
	}
	else
	{
		// Same problem as AddCameraYawRotation(). We need to move our Root so Camera matches new location (not Root).
		FVector LocalRootMoveDirection = MainCamera->GetRelativeLocation() * FVector(1.f, 1.f, 0.f);
		FVector RotatedDirection = NewRotation.RotateVector(LocalRootMoveDirection);

		SetActorLocation(NewLocation - RotatedDirection);
		SetActorRotation(NewRotation);
	}
}

FName AVirtualRealityPawn::GetCurrentControllersTypeName() const
{
	return CurrentControllersTypeName;
}

FVector AVirtualRealityPawn::GetCameraRelativeLocation() const
{
	return MainCamera->GetRelativeLocation();
}

FRotator AVirtualRealityPawn::GetCameraRelativeRotation() const
{
	return MainCamera->GetRelativeRotation();
}