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

	SetActorLocation(MainCameraLocationProjected - RotatedRootMoveDirection, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw + YawToAdd, 0.f), ETeleportType::TeleportPhysics);

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


// BEGIN INPUT

void AVirtualRealityPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(InputBindingName_Axis_Right_Thumbstick_X, this, &AVirtualRealityPawn::Input_Axis_Right_Thumbstick_X);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Right_Thumbstick_Y, this, &AVirtualRealityPawn::Input_Axis_Right_Thumbstick_Y);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Left_Thumbstick_X, this, &AVirtualRealityPawn::Input_Axis_Left_Thumbstick_X);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Left_Thumbstick_Y, this, &AVirtualRealityPawn::Input_Axis_Left_Thumbstick_Y);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Right_Trigger, this, &AVirtualRealityPawn::Input_Axis_Right_Trigger);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Left_Trigger, this, &AVirtualRealityPawn::Input_Axis_Left_Trigger);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Right_Grip, this, &AVirtualRealityPawn::Input_Axis_Right_Grip);
	PlayerInputComponent->BindAxis(InputBindingName_Axis_Left_Grip, this, &AVirtualRealityPawn::Input_Axis_Left_Grip);

	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Primary_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Primary_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Primary_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Primary_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Secondary_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Secondary_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Secondary_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Secondary_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Primary_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Primary_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Primary_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Primary_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Secondary_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Secondary_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Secondary_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Secondary_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Primary_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Primary_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Primary_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Primary_Touch, EButtonActionType::ReleasedTouch);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Secondary_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Secondary_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Secondary_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Secondary_Touch, EButtonActionType::ReleasedTouch);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Primary_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Primary_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Primary_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Primary_Touch, EButtonActionType::ReleasedTouch);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Secondary_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Secondary_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Secondary_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Secondary_Touch, EButtonActionType::ReleasedTouch);

	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Trigger_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Trigger_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Trigger_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Trigger_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Trigger_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Trigger_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Trigger_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Trigger_Touch, EButtonActionType::ReleasedTouch);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Trigger_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Trigger_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Trigger_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Trigger_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Trigger_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Trigger_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Trigger_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Trigger_Touch, EButtonActionType::ReleasedTouch);

	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Grip_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Grip_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Grip_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Grip_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Grip_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Grip_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Grip_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Grip_Touch, EButtonActionType::ReleasedTouch);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Grip_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Grip_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Grip_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Grip_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Grip_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Grip_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Grip_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Grip_Touch, EButtonActionType::ReleasedTouch);

	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Thumbstick_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Thumbstick_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Thumbstick_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Thumbstick_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Thumbstick_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Right_Thumbstick_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Right_Thumbstick_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Right_Thumbstick_Touch, EButtonActionType::ReleasedTouch);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Thumbstick_Press, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Thumbstick_Press, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Thumbstick_Press, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Thumbstick_Press, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Thumbstick_Touch, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Left_Thumbstick_Touch, EButtonActionType::Touched);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Left_Thumbstick_Touch, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Left_Thumbstick_Touch, EButtonActionType::ReleasedTouch);

	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Menu, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_Menu, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_Menu, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_Menu, EButtonActionType::ReleasedPress);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_System, EInputEvent::IE_Pressed, this, &AVirtualRealityPawn::Input_Button_System, EButtonActionType::Pressed);
	PlayerInputComponent->BindAction<InputActionType>(InputBindingName_Button_Action_Button_System, EInputEvent::IE_Released, this, &AVirtualRealityPawn::Input_Button_System, EButtonActionType::ReleasedPress);
}

const FName AVirtualRealityPawn::InputBindingName_Axis_Right_Thumbstick_X("Axis_Right_Thumbstick_X");
const FName AVirtualRealityPawn::InputBindingName_Axis_Right_Thumbstick_Y("Axis_Right_Thumbstick_Y");
const FName AVirtualRealityPawn::InputBindingName_Axis_Left_Thumbstick_X("Axis_Left_Thumbstick_X");
const FName AVirtualRealityPawn::InputBindingName_Axis_Left_Thumbstick_Y("Axis_Left_Thumbstick_Y");
const FName AVirtualRealityPawn::InputBindingName_Axis_Right_Trigger("Axis_Right_Trigger");
const FName AVirtualRealityPawn::InputBindingName_Axis_Left_Trigger("Axis_Left_Trigger");
const FName AVirtualRealityPawn::InputBindingName_Axis_Right_Grip("Axis_Right_Grip");
const FName AVirtualRealityPawn::InputBindingName_Axis_Left_Grip("Axis_Left_Grip");

const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Primary_Press("Button_Left_Primary_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Secondary_Press("Button_Left_Secondary_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Primary_Press("Button_Right_Primary_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Secondary_Press("Button_Right_Secondary_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Primary_Touch("Button_Left_Primary_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Secondary_Touch("Button_Left_Secondary_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Primary_Touch("Button_Right_Primary_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Secondary_Touch("Button_Right_Secondary_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Trigger_Touch("Button_Right_Trigger_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Trigger_Touch("Action_Button_Left_Trigger_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Trigger_Press("Action_Button_Right_Trigger_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Trigger_Press("Action_Button_Left_Trigger_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Grip_Touch("Action_Button_Right_Grip_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Grip_Touch("Action_Button_Left_Grip_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Grip_Press("Action_Button_Right_Grip_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Grip_Press("Action_Button_Left_Grip_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Thumbstick_Touch("Action_Button_Right_Thumbstick_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Thumbstick_Touch("Action_Button_Left_Thumbstick_Touch");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Right_Thumbstick_Press("Action_Button_Right_Thumbstick_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Left_Thumbstick_Press("Action_Button_Left_Thumbstick_Press");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_Menu("Button_Menu");
const FName AVirtualRealityPawn::InputBindingName_Button_Action_Button_System("Button_System");

// Axies

void AVirtualRealityPawn::Input_Axis_Right_Thumbstick_X(float Value) { if (RightHand) RightHand->PawnInput_Axis_Thumbstick_X(Value); }
void AVirtualRealityPawn::Input_Axis_Right_Thumbstick_Y(float Value) { if (RightHand) RightHand->PawnInput_Axis_Thumbstick_Y(Value); }
void AVirtualRealityPawn::Input_Axis_Left_Thumbstick_X(float Value) { if (LeftHand) LeftHand->PawnInput_Axis_Thumbstick_X(Value); }
void AVirtualRealityPawn::Input_Axis_Left_Thumbstick_Y(float Value) { if (LeftHand) LeftHand->PawnInput_Axis_Thumbstick_Y(Value); }
void AVirtualRealityPawn::Input_Axis_Right_Trigger(float Value) { if (RightHand) RightHand->PawnInput_Axis_Trigger(Value); }
void AVirtualRealityPawn::Input_Axis_Left_Trigger(float Value) { if (LeftHand) LeftHand->PawnInput_Axis_Trigger(Value); }
void AVirtualRealityPawn::Input_Axis_Right_Grip(float Value) { if (RightHand) RightHand->PawnInput_Axis_Grip(Value); }
void AVirtualRealityPawn::Input_Axis_Left_Grip(float Value) { if (LeftHand) LeftHand->PawnInput_Axis_Grip(Value); }

// Buttons

void AVirtualRealityPawn::Input_Button_Left_Primary_Press(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Primary(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Secondary_Press(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Secondary(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Primary_Press(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Primary(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Secondary_Press(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Secondary(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Primary_Touch(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Primary(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Secondary_Touch(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Secondary(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Primary_Touch(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Primary(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Secondary_Touch(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Secondary(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Trigger_Touch(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Trigger(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Trigger_Touch(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Trigger(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Trigger_Press(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Trigger(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Trigger_Press(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Trigger(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Grip_Touch(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Grip(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Grip_Touch(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Grip(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Grip_Press(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Grip(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Grip_Press(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Grip(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Thumbstick_Touch(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Thumbstick(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Thumbstick_Touch(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Thumbstick(EventType); }
void AVirtualRealityPawn::Input_Button_Right_Thumbstick_Press(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_Thumbstick(EventType); }
void AVirtualRealityPawn::Input_Button_Left_Thumbstick_Press(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Thumbstick(EventType); }
void AVirtualRealityPawn::Input_Button_Menu(EButtonActionType EventType) { if (LeftHand) LeftHand->PawnInput_Button_Menu(EventType); }
void AVirtualRealityPawn::Input_Button_System(EButtonActionType EventType) { if (RightHand) RightHand->PawnInput_Button_System(EventType); }