// Alex Smirnov 2020-2021

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "Engine/StreamableManager.h"

#include "GameInstWithStreamableManager.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTVRBASICS_API UGameInstWithStreamableManager : public UGameInstance
{
	GENERATED_BODY()
	
public:
	FStreamableManager& GetStreamableManager();

protected:
	FStreamableManager StreamableManager;
};
