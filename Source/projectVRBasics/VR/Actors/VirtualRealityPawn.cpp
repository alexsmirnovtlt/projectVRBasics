// Alex Smirnov 2020-2021


#include "VirtualRealityPawn.h"

#include "VirtualRealityMotionController.h"
#include "MotionControllerComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "IXRTrackingSystem.h"
#include "IXRSystemAssets.h"


AVirtualRealityPawn::AVirtualRealityPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Root
	auto NewRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = NewRootComponent;
	// VR Camera
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	MainCamera->AttachTo(RootComponent);

	// Cached attachment properties for controllers creation
	AttachmentRules.ScaleRule = EAttachmentRule::KeepWorld;
}

void AVirtualRealityPawn::BeginPlay()
{
	Super::BeginPlay();

	TrackingSystem = GEngine->XRSystem.Get();
	if (!ensure(TrackingSystem)) { return; }

	UE_LOG(LogTemp, Warning, TEXT("VR Headset Info: %s"), *TrackingSystem->GetVersionString()); // Log contains important headset info such as manufacturer and driver version 

	// Check that VR Headset is present and set tracking origin
	if (!InitHeadset()) return;

	InitMotionControllers();
}

bool AVirtualRealityPawn::InitHeadset()
{
	uint32 HMDCount = TrackingSystem->CountTrackedDevices(EXRTrackedDeviceType::HeadMountedDisplay);

	if (HMDCount != 1)
	{
		// TODO Handle no headset

		return false;
	}
	// Setting that our camera will change its local position relative to the real world floor
	TrackingSystem->SetTrackingOrigin(EHMDTrackingOrigin::Floor); // will work for everything except PS Move, EHMDTrackingOrigin::Eye for that

	return true;
}

void AVirtualRealityPawn::InitMotionControllers()
{
	if (bStartWithPlatformIndependentControllers)
	{
		SwitchMotionControllers(StartingControllerName);
	}
	else
	{
		FString HeadsetInfo = TrackingSystem->GetVersionString();
		for (auto& HeadsetType : ControllerTypes)
		{
			if (HeadsetInfo.Contains(HeadsetType.HeadsetName.ToString()))
			{
				SwitchMotionControllers(HeadsetType.HeadsetName);
			}
		}
	}
}

bool AVirtualRealityPawn::SwitchMotionControllers(FName NewProfileName)
{
	for (auto& HeadsetType : ControllerTypes)
	{
		if (HeadsetType.HeadsetName.IsEqual(NewProfileName))
		{
			auto LeftHandClass = HeadsetType.LeftController.LoadSynchronous();
			auto RightHandClass = HeadsetType.RightController.LoadSynchronous();
			if (ensure(LeftHandClass && RightHandClass))
			{
				CreateMotionController(true, LeftHandClass);
				CreateMotionController(false, RightHandClass);
			}

			return true;
		}
	}

	return false;
}

void AVirtualRealityPawn::CreateMotionController(bool bLeft, UClass* ClassToCreate)
{
	if (bLeft)
	{
		if (LeftHand)
		{
			LeftHand->Destroy();
		}

		LeftHand = GetWorld()->SpawnActor<AVirtualRealityMotionController>(ClassToCreate, FVector::ZeroVector, FRotator::ZeroRotator);
		LeftHand->AttachToComponent(RootComponent, AttachmentRules);
		LeftHand->SetOwner(this);
		LeftHand->InitialSetup(TEXT("Left"));
	}
	else
	{
		if (RightHand)
		{
			RightHand->Destroy();
		}

		RightHand = GetWorld()->SpawnActor<AVirtualRealityMotionController>(ClassToCreate, FVector::ZeroVector, FRotator::ZeroRotator);
		RightHand->AttachToComponent(RootComponent, AttachmentRules);
		RightHand->SetOwner(this);
		RightHand->InitialSetup(TEXT("Right"));
	}
}

void AVirtualRealityPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
/*
EHeadsetType AVirtualRealityPawn::DetectControllersType()
{
	// Should be reworked. Detecting by parsing headset info string. Should somehow be detecting from controllers themselves.

	FString HeadsetInfo = TrackingSystem->GetVersionString();
	UE_LOG(LogTemp, Warning, TEXT("VR Headset Info: %s"), *HeadsetInfo); // Log contains important headset info such as manufacturer and driver version 



	return EHeadsetType::Other;
}*/