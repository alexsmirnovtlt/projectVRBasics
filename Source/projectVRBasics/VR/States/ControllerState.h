// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "../Actors/Interfaces/VRPlayerInput.h"

#include "ControllerState.generated.h"

class AVirtualRealityMotionController;

/**
 * 
 */
UCLASS(Blueprintable)
class PROJECTVRBASICS_API UControllerState : public UObject, public IVRPlayerInput
{
	GENERATED_BODY()

public:

	void SetOwningController(AVirtualRealityMotionController* MotionController);
	void SetOtherControllerReference(UControllerState* OtherControllerReference);

	AVirtualRealityMotionController* GetOwningMotionController() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Override")
	uint8 GetControllerStateAsByte() const;

	void NotifyPairedControllerOfStateChange(bool bStateEntered);

	UFUNCTION(BlueprintCallable, Category = "Override")
	UControllerState* GetPairedControllerState();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Controller State")
	void Tick(float DeltaTime);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Controller State")
	void OnStateEnter();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Controller State")
	void OnStateExit();

	UFUNCTION(BlueprintCallable, Category = "Controller State")
	AActor* SpawnActor(TSubclassOf<AActor> ClassToSpawn);

protected:

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Controller State")
	void PairedControllerStateChanged(UControllerState* OtherControllerNewState, bool bEntered);

	UPROPERTY()
	TWeakObjectPtr<UControllerState> PairedController; // we are constantly cross-referencing both controller`s states so that should be a weak pointer

	UPROPERTY(BlueprintReadonly)
	AVirtualRealityMotionController* OwningMotionController;
};
