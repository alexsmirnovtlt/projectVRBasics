// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "VirtualRealityPawn.generated.h"

class UCameraComponent;
class IXRTrackingSystem;
class AVirtualRealityMotionController;

/*UENUM(BlueprintType)
enum class EHeadsetType : uint8 {
	Oculus = 0 UMETA(DisplayName = "Oculus"),
	Index = 1 UMETA(DisplayName = "Index"),
	Vive = 2 UMETA(DisplayName = "Vive"),
	Other = 3 UMETA(DisplayName = "Other")
};*/

USTRUCT(Blueprintable)
struct FControllerType
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FName HeadsetName;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AVirtualRealityMotionController> LeftController;
	UPROPERTY(EditAnywhere)
	TSoftClassPtr<AVirtualRealityMotionController> RightController;
};

UCLASS()
class PROJECTVRBASICS_API AVirtualRealityPawn : public APawn
{
	GENERATED_BODY()

public:
	AVirtualRealityPawn();

protected:
	virtual void BeginPlay() override;

public:	
	//virtual void Tick(float DeltaTime) override;
	//virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	FName GetCurrentControllersTypeName();

protected:
	// If not empty, overrides controller type that will be used from ControllerTypes. If is empty, headset info will be used to determine Headset Type. (f.e if we use same hands on any Headset, we define it here and in ControllerTypes)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	FName StartingControllerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	TArray<FControllerType> ControllerTypes;

	UPROPERTY()
	UCameraComponent* MainCamera;
	UPROPERTY()
	AVirtualRealityMotionController* LeftHand;
	UPROPERTY()
	AVirtualRealityMotionController* RightHand;

	FName CurrentControllersTypeName;

	FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false);;

	bool InitHeadset(IXRTrackingSystem& TrackingSystem);
	void InitMotionControllers(IXRTrackingSystem& TrackingSystem);
	void CreateMotionController(bool bLeft, UClass* ClassToCreate);

	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	bool SwitchMotionControllers(FName NewProfileName);

private:
	FName StartupLevelName = TEXT("StartUpVRMap");
};
