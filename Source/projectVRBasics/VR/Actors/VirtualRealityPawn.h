// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "VirtualRealityPawn.generated.h"

class UCameraComponent;
class AVirtualRealityMotionController;

UENUM(BlueprintType)
enum class EHeadsetType : uint8 {
	Oculus = 0 UMETA(DisplayName = "Oculus"),
	Index = 1 UMETA(DisplayName = "Index"),
	Vive = 2 UMETA(DisplayName = "Vive"),
	Other = 3 UMETA(DisplayName = "Other")
};

USTRUCT(BlueprintType)
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
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	FName GetCurrentControllersTypeName();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	bool bStartWithPlatformIndependentControllers;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup", meta = (EditCondition = "bStartWithPlatformIndependentControllers"))
	FName StartingControllerName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Setup")
	TArray<FControllerType> ControllerTypes;

	class IXRTrackingSystem* TrackingSystem;

	UCameraComponent* MainCamera;
	AVirtualRealityMotionController* LeftHand;
	AVirtualRealityMotionController* RightHand;

	FName CurrentControllersTypeName;

	FAttachmentTransformRules AttachmentRules = FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false);;

	bool InitHeadset();
	void InitMotionControllers();
	void CreateMotionController(bool bLeft, UClass* ClassToCreate);
	UFUNCTION(BlueprintCallable, Category = "VR Setup")
	bool SwitchMotionControllers(FName NewProfileName);
};
