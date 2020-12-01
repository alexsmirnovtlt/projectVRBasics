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
	void AddPairedController(AVirtualRealityMotionController* AnotherMotionController);

	UFUNCTION()
	UControllerState* GetControllerState();

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Motion Controller Setup")
	TSubclassOf<UControllerState> StartStateClass;

	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* MotionController;

	UPROPERTY(BlueprintReadonly)
	AVirtualRealityPawn* OwningPawn;

	UPROPERTY(BlueprintReadonly)
	UControllerState* ControllerState;
};
