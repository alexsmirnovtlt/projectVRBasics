// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "VirtualRealityMotionController.generated.h"

UCLASS(abstract, Blueprintable)
class PROJECTVRBASICS_API AVirtualRealityMotionController : public AActor
{
	GENERATED_BODY()
	
public:	
	AVirtualRealityMotionController();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void InitialSetup(FName MotionSource);

protected:
	UPROPERTY(VisibleAnywhere)
	class UMotionControllerComponent* MotionController;
};
