// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "VRCheckGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTVRBASICS_API AVRCheckGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:
	virtual void StartPlay() override;
};
