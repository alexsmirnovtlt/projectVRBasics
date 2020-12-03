// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"


#include "VRTeleportLogic.generated.h"

struct FStreamableHandle;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTVRBASICS_API UVRTeleportLogic : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void Initialize();
	UFUNCTION(BlueprintCallable)
	void HandleDestruction();

	UFUNCTION(BlueprintCallable)
	AActor* GetArrowActor();
	UFUNCTION(BlueprintCallable)
	AActor* GetTeleportBeamActor();

protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AActor> TeleportArrowClass;
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<AActor> TeleportBeamPartClass;

	TSharedPtr<FStreamableHandle> TeleportArrowHandle;
	TSharedPtr<FStreamableHandle> TeleportBeamHandle;

	UPROPERTY()
	AActor* TeleportArrowActor;
	UPROPERTY()
	AActor* TeleportBeamPartActor;

	UFUNCTION()
	void OnAssetLoaded();
};
