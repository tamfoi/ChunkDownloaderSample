// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomGameInstance.h"

#include "ChunkDownloader.h"
#include "Misc/CoreDelegates.h"
#include "AssetRegistryModule.h"

void UCustomGameInstance::Init()
{
    Super::Init();
}

void UCustomGameInstance::Shutdown()
{
    Super::Shutdown();

    // Shut down ChunkDownloader
    FChunkDownloader::Shutdown();
}

void UCustomGameInstance::GetLoadingProgress(int32& BytesDownloaded, int32& TotalBytesToDownload, float& DownloadPercent, int32& ChunksMounted, int32& TotalChunksToMount, float& MountPercent) const
{
    //Get a reference to ChunkDownloader
    TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

    //Get the loading stats struct
    FChunkDownloader::FStats LoadingStats = Downloader->GetLoadingStats();

    //Get the bytes downloaded and bytes to download
    BytesDownloaded = LoadingStats.BytesDownloaded;
    TotalBytesToDownload = LoadingStats.TotalBytesToDownload;

    //Get the number of chunks mounted and chunks to download
    ChunksMounted = LoadingStats.ChunksMounted;
    TotalChunksToMount = LoadingStats.TotalChunksToMount;

    //Calculate the download and mount percent using the above stats
    DownloadPercent = (float)BytesDownloaded / (float)TotalBytesToDownload;
    MountPercent = (float)ChunksMounted / (float)TotalChunksToMount;
}

void UCustomGameInstance::OnLoadingModeComplete(bool bSuccess)
{
    OnDownloadComplete(bSuccess);
}

void UCustomGameInstance::OnMountComplete(bool bSuccess)
{
    OnFetchChunk.Broadcast(bSuccess);
}

void UCustomGameInstance::FetchBuildManifest()
{
    const FString DeploymentName = "DeploymentName";
    const FString ContentBuildId = "BuildVersion";

    // initialize the chunk downloader
    TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetOrCreate();
    Downloader->Initialize("PlatformName", 8);

    // load the cached build ID
    if (Downloader->LoadCachedBuild(DeploymentName))
    {
        UE_LOG(LogTemp, Display, TEXT("Cached Build Succeeded"));
    }
    else {
        UE_LOG(LogTemp, Display, TEXT("Cached Build Failed"));
    }

    // update the build manifest file
    TFunction<void(bool)> UpdateCompleteCallback = [this](bool bSuccess)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Display, TEXT("Manifest Update Succeeded"));
        }
        else {
            UE_LOG(LogTemp, Display, TEXT("Manifest Update Failed"));
        }

        bIsDownloadManifestUpToDate = bSuccess;
        OnFetchBuildManifest.Broadcast(bSuccess);

    };

    Downloader->UpdateBuild(DeploymentName, ContentBuildId, UpdateCompleteCallback);
}

bool UCustomGameInstance::FetchChunk()
{
    // make sure the download manifest is up to date
    if (bIsDownloadManifestUpToDate)
    {
        // get the chunk downloader
        TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

        // report current chunk status
        for (int32 ChunkID : ChunkDownloadList)
        {
            int32 ChunkStatus = static_cast<int32>(Downloader->GetChunkStatus(ChunkID));
            UE_LOG(LogTemp, Display, TEXT("Chunk %i status: %i"), ChunkID, ChunkStatus);
        }

        TFunction<void(bool bSuccess)> DownloadCompleteCallback = [&](bool bSuccess) {OnDownloadComplete(bSuccess); };
        Downloader->DownloadChunks(ChunkDownloadList, DownloadCompleteCallback, 1);

        // start loading mode
        TFunction<void(bool bSuccess)> LoadingModeCompleteCallback = [&](bool bSuccess) {OnLoadingModeComplete(bSuccess); };
        Downloader->BeginLoadingMode(LoadingModeCompleteCallback);
        return true;
    }

    // we couldn't contact the server to validate our manifest, so we can't patch
    UE_LOG(LogTemp, Display, TEXT("Manifest Update Failed.Can't patch the game"));

    return false;

}

void UCustomGameInstance::OnDownloadComplete(bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Display, TEXT("Download complete"));

        // get the chunk downloader
        TSharedRef<FChunkDownloader> Downloader = FChunkDownloader::GetChecked();

        FJsonSerializableArrayInt DownloadedChunks;

        for (int32 ChunkID : ChunkDownloadList)
        {
            DownloadedChunks.Add(ChunkID);
        }

        //Mount the chunks
        TFunction<void(bool bSuccess)> MountCompleteCallback = [&](bool bSuccess) {OnMountComplete(bSuccess); };
        Downloader->MountChunks(DownloadedChunks, MountCompleteCallback);

        OnFetchChunk.Broadcast(true);

    }
    else
    {

        UE_LOG(LogTemp, Display, TEXT("Load process failed"));

        // call the delegate
        OnFetchChunk.Broadcast(false);
    }
}
