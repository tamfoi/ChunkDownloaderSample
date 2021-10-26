// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CustomGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatchCompleteDelegate, bool, Succeeded);

/**
 * 
 */
UCLASS()
class UCustomGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Overrides */
	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY(BlueprintAssignable, Category = "Patching");
	FPatchCompleteDelegate OnFetchBuildManifest;

	UPROPERTY(BlueprintAssignable, Category = "Patching");
	FPatchCompleteDelegate OnFetchChunk;

	UFUNCTION(BlueprintCallable, Category = "Patching")
	void FetchBuildManifest();

	UFUNCTION(BlueprintCallable, Category = "Patching")
	bool FetchChunk();

protected:
	//Tracks Whether or not our local manifest file is up to date with the one hosted on our website
	bool bIsDownloadManifestUpToDate = false;

	UPROPERTY(EditDefaultsOnly, Category = "Patching")
	TArray<int32> ChunkDownloadList;

	UFUNCTION(BlueprintPure, Category = "Patching|Stats")
	void GetLoadingProgress(int32& FilesDownloaded, int32& TotalFilesToDownload, float& DownloadPercent, int32& ChunksMounted, int32& TotalChunksToMount, float& MountPercent) const;

	/** Called when the chunk download process finishes */
	void OnDownloadComplete(bool bSuccess);

	/** Called whenever ChunkDownloader's loading mode is finished*/
	void OnLoadingModeComplete(bool bSuccess);

	/** Called when ChunkDownloader finishes mounting chunks */
	void OnMountComplete(bool bSuccess);
};
