// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "VirtualRealityMotionController.generated.h"

class AVirtualRealityPawn;
class UControllerState;
class USplineComponent;


UCLASS(Blueprintable, abstract)
class PROJECTVRBASICS_API AVirtualRealityMotionController : public AActor
{
	GENERATED_BODY()
	
public:	
	AVirtualRealityMotionController();

protected:
	virtual void BeginPlay() override;

public:	

	void InitialSetup(AVirtualRealityPawn* Owner, bool IsLeft, bool IsPrimary);
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
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Motion Controller")
	USplineComponent* GetSplineComponent() const;
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Motion Controller")
	bool IsRightHandController();

	UFUNCTION(BlueprintImplementableEvent, Category = "Motion Controller Events")
	void OnPawnTeleported(bool bCameraViewOnly);

	UFUNCTION()
	UControllerState* GetControllerState() const;

	UFUNCTION(BlueprintCallable, Category = "Motion Controller")
	AVirtualRealityPawn* GetVRPawn() const;

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup")
	TSubclassOf<UControllerState> StartStateClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadonly, Category = "Motion Controller")
	class UMotionControllerComponent* MotionController;

	UPROPERTY(BlueprintReadonly, Category = "Motion Controller")
	AVirtualRealityPawn* OwningVRPawn;

	UPROPERTY(BlueprintReadonly, Category = "Motion Controller")
	UControllerState* ControllerState;

	UPROPERTY(BlueprintReadonly, Category = "Motion Controller")
	bool IsControllerPrimary;

	UFUNCTION(BlueprintImplementableEvent, Category = "Motion Controller Events")
	void OnDoneInitByPawn();

	bool IsRightController;

	UPROPERTY()
	USplineComponent* SplineComponent;
};
