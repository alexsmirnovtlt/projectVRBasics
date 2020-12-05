// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "VirtualRealityPawn.generated.h"

class UCameraComponent;
class IXRTrackingSystem;
class AVirtualRealityMotionController;


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
	FName GetCurrentControllersTypeName() const;

	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void AddCameraYawRotation(float YawToAdd); // used in teleport state when player may move view left or right. Pawn itself should not be affected by this view change
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	void TeleportToLocation(FVector NewLocation, FRotator NewRotation, bool bResetLocalPosition = true); //  used in teleport state when player teleports
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	FVector GetCameraRelativeLocation() const;
	UFUNCTION(BlueprintCallable, Category = "VR Movement")
	FRotator GetCameraRelativeRotation() const;

protected:
	// If not empty, overrides controller type that will be used from ControllerTypes. If is empty, headset info will be used to determine Headset Type. (f.e if we use same hands on any Headset, we define it here and in ControllerTypes)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	FName StartingControllerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	TArray<FControllerType> ControllerTypes;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* PawnRootComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
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
