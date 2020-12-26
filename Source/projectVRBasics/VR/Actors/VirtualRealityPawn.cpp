// Alex Smirnov 2020-2021


#include "VirtualRealityPawn.h"

#include "VirtualRealityMotionController.h"
#include "Components/CapsuleComponent.h"
#include "MotionControllerComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "IXRTrackingSystem.h"
#include "IXRSystemAssets.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include "../States/ControllerState.h"


AVirtualRealityPawn::AVirtualRealityPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root
	PawnRootComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RootComponent"));
	PawnRootComponent->SetCapsuleSize(40.f, 92.f);
	RootComponent = PawnRootComponent;

	VRRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("VRRootComponent"));
	VRRootComponent->SetupAttachment(RootComponent);
	VRRootComponent->SetRelativeLocation(FVector(0.f, 0.f, -1 * PawnRootComponent->GetScaledCapsuleHalfHeight()));
	
	// VR Camera
	MainCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	MainCamera->SetupAttachment(VRRootComponent);
	MainCamera->SetRelativeLocation(FVector(0.f, 0.f, PawnRootComponent->GetScaledCapsuleHalfHeight()));

	// Cached attachment properties for controllers creation
	AttachmentRules.ScaleRule = EAttachmentRule::KeepWorld;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	StartFadeTimeSec = 1.f;
	RightControllerIsPrimary = true;
}

void AVirtualRealityPawn::BeginPlay()
{
	Super::BeginPlay();

	// Fading screen because our camera probably wont be exactly at starting position
	if (auto PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
	}

	// Starting up timer to wait for headset to update its position
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle_StartCameraFade,
		this,
		&AVirtualRealityPawn::OnStartTimerEnd,
		StartFadeTimeSec,
		false
	);
}

void AVirtualRealityPawn::Destroyed()
{
	// Releasing resources that belong to current hands. TODO check maybe its released automatically when StreamableHandle gets destroyed
	if (LeftHandStreamableHandle.IsValid())
	{
		LeftHandStreamableHandle.Get()->ReleaseHandle();
	}
	if (RightHandStreamableHandle.IsValid())
	{
		RightHandStreamableHandle.Get()->ReleaseHandle();
	}
}

void AVirtualRealityPawn::OnStartTimerEnd()
{
	TSharedPtr<IXRTrackingSystem, ESPMode::ThreadSafe> TrackingSystem = GEngine->XRSystem;
	if (!ensure(TrackingSystem.IsValid())) { return; }

	TeleportToLocation(VRRootComponent->GetComponentLocation(), GetActorRotation()); // so player will be standing at spawn location even if he is not standing at the center of tracked zone irl

	// Check that VR Headset is present and set tracking origin
	if (!InitHeadset(TrackingSystem.Get())) return;
	
	// Trying to create motion controlles using StartingControllerName if not none, or detecting Headset type using info from TrackingSystem
	InitMotionControllers(TrackingSystem.Get());

	if (auto PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->PlayerCameraManager->StartCameraFade(1.f, 0.f, StartFadeTimeSec, FLinearColor::Black, false, true);
	}
}

bool AVirtualRealityPawn::InitHeadset(IXRTrackingSystem* TrackingSystem)
{
	uint32 HMDCount = TrackingSystem->CountTrackedDevices(EXRTrackedDeviceType::HeadMountedDisplay);

	if (HMDCount != 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Headset was not detected"));
		UGameplayStatics::OpenLevel(GetWorld()->GetGameInstance(), StartupLevelName);
		return false;
	}
	// Setting that our camera will change its local position relative to the real world floor
	TrackingSystem->SetTrackingOrigin(EHMDTrackingOrigin::Floor); // will work for everything except PS Move, EHMDTrackingOrigin::Eye for that

	return true;
}

void AVirtualRealityPawn::InitMotionControllers(IXRTrackingSystem* TrackingSystem)
{
	// Either creating hands that defined in StartingControllerName or getting headset info and creating controllers using that info
	bool bSuccessControllersCreation = false;

	if (!StartingControllerName.IsNone()) bSuccessControllersCreation = SwitchMotionControllersByName(StartingControllerName);
	
	if(!bSuccessControllersCreation)
	{
		FString HeadsetInfo = TrackingSystem->GetVersionString();
		for (auto& HeadsetType : ControllerTypes)
		{
			if (HeadsetInfo.Contains(HeadsetType.HeadsetName.ToString()))
			{
				bSuccessControllersCreation = SwitchMotionControllersByName(HeadsetType.HeadsetName);
			}
		}
	}

	if (!bSuccessControllersCreation) UE_LOG(LogTemp, Error, TEXT("Could not find Motion Controller Class to Spawn. Check StartingControllerName and/or ControllerTypes fields at %s"), *this->GetClass()->GetName());
}

bool AVirtualRealityPawn::SwitchMotionControllersByName(FName NewProfileName)
{
	for (auto& HeadsetType : ControllerTypes)
	{
		if (HeadsetType.HeadsetName.IsEqual(NewProfileName))
		{
			CurrentControllersTypeName = NewProfileName;
			SwitchMotionControllersByClass(HeadsetType.LeftController, HeadsetType.RightController);

			return true;
		}
	}

	CurrentControllersTypeName = TEXT("None");
	return false;
}

void AVirtualRealityPawn::SwitchMotionControllersByClass(TSoftClassPtr<AVirtualRealityMotionController> LeftHandClassSoftObjPtr, TSoftClassPtr<AVirtualRealityMotionController> RightHandClassSoftObjPtr)
{
	// Removing previous controllers and notifying their states to exit
	if (LeftHand)
	{
		auto ControllerState = LeftHand->GetControllerState();
		if (ControllerState)
		{
			ControllerState->OnStateExit();
		}
		if (LeftHandStreamableHandle.IsValid())
		{
			LeftHandStreamableHandle.Get()->ReleaseHandle();
		}
		LeftHand->Destroy();
	}
	if (RightHand)
	{
		auto ControllerState = RightHand->GetControllerState();
		if (ControllerState)
		{
			ControllerState->OnStateExit();
		}
		if (RightHandStreamableHandle.IsValid())
		{
			RightHandStreamableHandle.Get()->ReleaseHandle();
		}
		RightHand->Destroy();
	}

	// Async loading our hand class. If same class used for both hands, loading it once
	auto AssetManager = UAssetManager::GetIfValid();
	if (AssetManager)
	{
		FStreamableManager& StreamableManager = AssetManager->GetStreamableManager();

		if (LeftHandClassSoftObjPtr.GetAssetName().Equals(RightHandClassSoftObjPtr.GetAssetName()))
		{
			FSimpleDelegate HandClassDelegate = FStreamableDelegate::CreateUObject(this, &AVirtualRealityPawn::OnHandAssetLoadDone, true, true);
			LeftHandStreamableHandle = StreamableManager.RequestAsyncLoad(LeftHandClassSoftObjPtr.ToSoftObjectPath(), HandClassDelegate);
		}
		else
		{
			FSimpleDelegate HandClassDelegate = FStreamableDelegate::CreateUObject(this, &AVirtualRealityPawn::OnHandAssetLoadDone, true, false);
			LeftHandStreamableHandle = StreamableManager.RequestAsyncLoad(LeftHandClassSoftObjPtr.ToSoftObjectPath(), HandClassDelegate);

			HandClassDelegate = FStreamableDelegate::CreateUObject(this, &AVirtualRealityPawn::OnHandAssetLoadDone, false, false);
			RightHandStreamableHandle = StreamableManager.RequestAsyncLoad(RightHandClassSoftObjPtr.ToSoftObjectPath(), HandClassDelegate);
		}
	}
}

void AVirtualRealityPawn::OnHandAssetLoadDone(bool bLeft, bool bUseForBothHands)
{
	UClass* ClassToCreate = nullptr;

	if (bLeft && LeftHandStreamableHandle.Get())
	{
		if (LeftHandStreamableHandle.Get()->HasLoadCompleted())
		{
			ClassToCreate = Cast<UClass>(LeftHandStreamableHandle.Get()->GetLoadedAsset());

			// creating the same controller for the right hand too
			if (bUseForBothHands) CreateMotionController(false, ClassToCreate);
		}
	}
	else if (!bLeft && RightHandStreamableHandle.Get())
	{
		if (RightHandStreamableHandle.Get()->HasLoadCompleted())
		{
			ClassToCreate = Cast<UClass>(RightHandStreamableHandle.Get()->GetLoadedAsset());
		}
	}

	CreateMotionController(bLeft, ClassToCreate);

	if (LeftHand && RightHand) LeftHand->PairControllers(RightHand);
}

void AVirtualRealityPawn::CreateMotionController(bool bLeft, UClass* ClassToCreate)
{
	if (!ClassToCreate) return;

	auto NewHandController = GetWorld()->SpawnActor<AVirtualRealityMotionController>(ClassToCreate, GetActorLocation(), FRotator::ZeroRotator);
	NewHandController->AttachToComponent(VRRootComponent, AttachmentRules);

	NewHandController->InitialSetup(this, bLeft, !RightControllerIsPrimary);

	if (bLeft) LeftHand = NewHandController;
	else RightHand = NewHandController;
}

void AVirtualRealityPawn::AddCameraYawRotation(float YawToAdd)
{
	// Main problem is we cant just rotate Camera component. So we are making that root component rotates around current camera position so camera may stay in the same place but change root rotation because of a root component 
	// If we just rotate Camera component or root, if we are not standing right at 0,0 local position in the real world, we wil be changing position as we rotate

	if (LeftHand) LeftHand->OnPawnTeleport(true, true);
	if (RightHand) RightHand->OnPawnTeleport(true, true);

	FVector MainCameraLocationProjected = MainCamera->GetComponentLocation() * FVector(1.f, 1.f, 0.f); // Projecting to X, Y, dont need Z
	FVector RootMoveDirection = MainCameraLocationProjected - GetActorLocation();
	FVector RotatedRootMoveDirection = FRotator(0.f, YawToAdd, 0.f).RotateVector(RootMoveDirection);

	SetActorLocation(MainCameraLocationProjected - RotatedRootMoveDirection, false, nullptr, ETeleportType::ResetPhysics);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw + YawToAdd, 0.f), ETeleportType::ResetPhysics);

	if (LeftHand) LeftHand->OnPawnTeleport(false, true);
	if (RightHand) RightHand->OnPawnTeleport(false, true);
}

void AVirtualRealityPawn::TeleportToLocation(FVector NewLocation, FRotator NewRotation)
{
	// Same problem as AddCameraYawRotation(). We need to move our Root so Camera matches new location (not Root).
	
	if (LeftHand) LeftHand->OnPawnTeleport(true, false);
	if (RightHand) RightHand->OnPawnTeleport(true, false);

	FVector LocalRootMoveDirection = MainCamera->GetRelativeLocation() * FVector(1.f, 1.f, 0.f);
	FVector RotatedDirection = NewRotation.RotateVector(LocalRootMoveDirection);

	SetActorLocation(NewLocation - RotatedDirection + FVector(0.f, 0.f, PawnRootComponent->GetScaledCapsuleHalfHeight()),false, nullptr, ETeleportType::ResetPhysics);
	SetActorRotation(NewRotation, ETeleportType::ResetPhysics);

	if (LeftHand) LeftHand->OnPawnTeleport(false, false);
	if (RightHand) RightHand->OnPawnTeleport(false, false);
}

FName AVirtualRealityPawn::GetCurrentControllersTypeName() const
{
	return CurrentControllersTypeName;
}

FTransform AVirtualRealityPawn::GetCameraRelativeTransform() const
{
	return MainCamera->GetRelativeTransform();
}

FTransform AVirtualRealityPawn::GetCameraWorldTransform() const
{
	return MainCamera->GetComponentTransform();
}