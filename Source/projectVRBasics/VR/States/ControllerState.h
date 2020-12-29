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
	UControllerState* GetPairedControllerState();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Events")
	void OnStateEnter();
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Events")
	void OnStateExit();

protected:

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "VR Controller Input")
	void PairedControllerStateChanged(UControllerState* OtherControllerNewState, bool bEntered);

	UPROPERTY(BlueprintReadonly)
	TWeakObjectPtr<UControllerState> PairedController; // we are constantly cross-referencing both controller`s states so that should be a weak pointer

	UPROPERTY(BlueprintReadonly)
	AVirtualRealityMotionController* OwningMotionController;
};
