// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "VirtualRealityMotionController.generated.h"

class AVirtualRealityPawn;
class UControllerState;

UCLASS(Blueprintable, abstract)
class PROJECTVRBASICS_API AVirtualRealityMotionController : public AActor
{
	GENERATED_BODY()
	
public:	
	AVirtualRealityMotionController();

protected:
	virtual void BeginPlay() override;

public:	
	//virtual void Tick(float DeltaTime) override;

	void InitialSetup(AVirtualRealityPawn* Owner, FName MotionSource);
	void PairControllers(AVirtualRealityMotionController* AnotherMotionController);

	UFUNCTION(BlueprintCallable, Category = "Motion Controller Setup")
	void ChangeState(TSubclassOf<UControllerState> NewStateClass, bool NotifyPairedControllerIfAble = true); 
	UFUNCTION(BlueprintCallable, Category = "Motion Controller Setup")
	void ChangeToDefaultState(bool NotifyPairedControllerIfAble = true);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FVector GetControllerWorldOriginLocation() const;
	virtual FVector GetControllerWorldOriginLocation_Implementation() const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	FRotator GetControllerWorldOriginRotation() const;
	virtual FRotator GetControllerWorldOriginRotation_Implementation() const;
	//For that 2 functions above we are making sure that Origin will return this Actor location and rotation but it may be overridden in BP.
	
	UFUNCTION()
	UControllerState* GetControllerState() const;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup")
	TSubclassOf<UControllerState> StartStateClass;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* MotionController;

	UPROPERTY(BlueprintReadonly)
	AVirtualRealityPawn* OwningVRPawn;

	UPROPERTY(BlueprintReadonly)
	UControllerState* ControllerState;
};
