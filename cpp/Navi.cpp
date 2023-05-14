#include <stdio.h>
#include <string>
#include <list>
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "Recast.h"
#include "fastlz.h"
#include "Filelist.h"
#include "Navi.h"

// check if is 64bit
int COMPILE_TEST[sizeof(void*) == 8 ? 1 : -1];

const int TILECACHESET_MAGIC = 'W'<<24 | 'L'<<16 | 'R'<<8 | 'D';
const int TILECACHESET_VERSION = 1;
const int MAX_POLYS = 1024;

const char* sVolumeTag = "Volume:";
const char* sAreaTag = "\tarea:";
const char* sHminTag = "\thmin:";
const char* sHmaxTag = "\thmax:";
const char* sNvertsTag = "\tnverts:";
const char* sVertTag = "\t\tvert:";
const char* sNlinkTag = "\tnlink:";
const char* sLinkTag = "\t\tlink:";
const char* sRegionTreeTag = "RegionTree:";
const char* sAABBTag = "\tAABB:";
const char* sTreeDeepTag = "\tDeep:";

/////////////////////////////////////////////////////////////////
// VolumeRegion
bool VolumeRegion::IsContain(float x, float z) const
{
    const int vertSize = (const int)verts.size();
    for (int i = 0, j = vertSize - 1; i < vertSize; j = i++)
    {
        const Vector3& a = verts[j];
        const Vector3& b = verts[i];
        const float cx = x - a.x;
        const float cz = z - a.z;
        const float bx = b.x - a.x;
        const float bz = b.z - a.z;
        const float result = cz * bx - cx * bz;
        if (result <= 0)
            return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////
// TileCacheSetHeader
struct TileCacheSetHeader
{
    int magic;
    int version;
    int numTiles;
    dtNavMeshParams meshParams;
    dtTileCacheParams cacheParams;
};

struct TileCacheTileHeader
{
    dtCompressedTileRef tileRef;
    int dataSize;
};

/////////////////////////////////////////////////////////////////
// FastLZCompressor
struct FastLZCompressor : public dtTileCacheCompressor
{
    virtual ~FastLZCompressor();
    
    virtual int maxCompressedSize(const int bufferSize)
    {
        return (int)(bufferSize* 1.05f);
    }
    
    virtual dtStatus compress(const unsigned char* buffer, const int bufferSize,
                              unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize)
    {
        *compressedSize = fastlz_compress((const void *const)buffer, bufferSize, compressed);
        return DT_SUCCESS;
    }
    
    virtual dtStatus decompress(const unsigned char* compressed, const int compressedSize,
                                unsigned char* buffer, const int maxBufferSize, int* bufferSize)
    {
        *bufferSize = fastlz_decompress(compressed, compressedSize, buffer, maxBufferSize);
        return *bufferSize < 0 ? DT_FAILURE : DT_SUCCESS;
    }
};

FastLZCompressor::~FastLZCompressor()
{
    // Defined out of line to fix the weak v-tables warning
}

/////////////////////////////////////////////////////////////////
// LinearAllocator
struct LinearAllocator : public dtTileCacheAlloc
{
    unsigned char* buffer;
    size_t capacity;
    size_t top;
    size_t high;
    
    LinearAllocator(const size_t cap) : buffer(0), capacity(0), top(0), high(0)
    {
        resize(cap);
    }
    
    virtual ~LinearAllocator();
    
    void resize(const size_t cap)
    {
        if (buffer) dtFree(buffer);
        buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
        capacity = cap;
    }
    
    virtual void reset()
    {
        high = dtMax(high, top);
        top = 0;
    }
    
    virtual void* alloc(const size_t size)
    {
        if (!buffer)
            return 0;
        if (top+size > capacity)
            return 0;
        unsigned char* mem = &buffer[top];
        top += size;
        return mem;
    }
    
    virtual void free(void* /*ptr*/)
    {
        // Empty
    }
};

LinearAllocator::~LinearAllocator()
{
    // Defined out of line to fix the weak v-tables warning
    dtFree(buffer);
}

/////////////////////////////////////////////////////////////////
// MeshProcess
struct MeshProcess : public dtTileCacheMeshProcess
{
    inline MeshProcess()
    {
    }
    
    virtual ~MeshProcess();
    
    virtual void process(struct dtNavMeshCreateParams* params,
                         unsigned char* polyAreas, unsigned short* polyFlags)
    {
        // Update poly flags from areas.
        for (int i = 0; i < params->polyCount; ++i)
        {
            if (polyAreas[i] == DT_TILECACHE_WALKABLE_AREA)
                polyAreas[i] = POLYAREA_GROUND;
            
            if (polyAreas[i] == POLYAREA_GROUND ||
                polyAreas[i] == POLYAREA_GRASS ||
                polyAreas[i] == POLYAREA_ROAD)
            {
                polyFlags[i] = POLYFLAGS_WALK;
            }
            else if (polyAreas[i] == POLYAREA_WATER)
            {
                polyFlags[i] = POLYFLAGS_SWIM;
            }
            else if (polyAreas[i] == POLYAREA_DOOR)
            {
                polyFlags[i] = POLYFLAGS_WALK | POLYFLAGS_DOOR;
            }
        }
    }
};

MeshProcess::~MeshProcess()
{
    // Defined out of line to fix the weak v-tables warning
}

/////////////////////////////////////////////////////////////////
// Navi
Navi::Navi()
:mNavMesh(nullptr)
,mNavQuery(nullptr)
,mTileCache(nullptr)
,mSearchedPolyCount(0)
,mPathCount(0)
,mDefaultPolySize(2, 4, 2)
{
    mAlloc = new LinearAllocator(32000);
    mComp = new FastLZCompressor;
    mProc = new MeshProcess;
    
    mFilter = new dtQueryFilter;
    mFilter->setIncludeFlags(POLYFLAGS_WALK);
    mFilter->setExcludeFlags(POLYFLAGS_DOOR);
    
    mSearchPolys = new dtPolyRef[MAX_POLYS];
    mPath = new Vector3[MAX_POLYS];
}

Navi::~Navi()
{
    dtFreeNavMeshQuery(mNavQuery);
    dtFreeNavMesh(mNavMesh);
    dtFreeTileCache(mTileCache);
    
    delete mPath;
    delete mSearchPolys;
    delete mFilter;
    
    delete mAlloc;
    delete mComp;
    delete mProc;
    
    for (int i = 0; i < mRegions.size(); ++i)
        delete mRegions[i];
}

bool Navi::LoadMesh(const char* path)
{
    dtFreeNavMeshQuery(mNavQuery);
    dtFreeNavMesh(mNavMesh);
    dtFreeTileCache(mTileCache);

    FILE* fp = fopen(path, "rb");
    if (!fp)
        return false;
    
    // Read header.
    TileCacheSetHeader header;
    size_t headerReadReturnCode = fread(&header, sizeof(TileCacheSetHeader), 1, fp);
    if( headerReadReturnCode != 1)
    {
        // Error or early EOF
        fclose(fp);
        return false;
    }
    if (header.magic != TILECACHESET_MAGIC)
    {
        fclose(fp);
        return false;
    }
    if (header.version != TILECACHESET_VERSION)
    {
        fclose(fp);
        return false;
    }
    
    mNavMesh = dtAllocNavMesh();
    if (!mNavMesh)
    {
        fclose(fp);
        return false;
    }
    dtStatus status = mNavMesh->init(&header.meshParams);
    if (dtStatusFailed(status))
    {
        fclose(fp);
        return false;
    }
    
    mTileCache = dtAllocTileCache();
    if (!mTileCache)
    {
        fclose(fp);
        return false;
    }
    
    status = mTileCache->init(&header.cacheParams, mAlloc, mComp, mProc);
    if (dtStatusFailed(status))
    {
        fclose(fp);
        return false;
    }
    
    // Read tiles.
    for (int i = 0; i < header.numTiles; ++i)
    {
        TileCacheTileHeader tileHeader;
        size_t tileHeaderReadReturnCode = fread(&tileHeader, sizeof(tileHeader), 1, fp);
        if( tileHeaderReadReturnCode != 1)
        {
            // Error or early EOF
            fclose(fp);
            return false;
        }
        if (!tileHeader.tileRef || !tileHeader.dataSize)
            break;
        
        unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
        if (!data) break;
        memset(data, 0, tileHeader.dataSize);
        size_t tileDataReadReturnCode = fread(data, tileHeader.dataSize, 1, fp);
        if( tileDataReadReturnCode != 1)
        {
            // Error or early EOF
            dtFree(data);
            fclose(fp);
            return false;
        }
        
        dtCompressedTileRef tile = 0;
        dtStatus addTileStatus = mTileCache->addTile(data, tileHeader.dataSize, DT_COMPRESSEDTILE_FREE_DATA, &tile);
        if (dtStatusFailed(addTileStatus))
        {
            dtFree(data);
        }
        
        if (tile)
            mTileCache->buildNavMeshTile(tile, mNavMesh);
    }
    
    fclose(fp);
    
    mNavQuery = dtAllocNavMeshQuery();
    mNavQuery->init(mNavMesh, 2048);
    return true;
}

void Navi::VolumesClear(void* volumes)
{
    if (volumes == &mDoors)
    {
        mDoors.clear();
        mDoorMap.clear();
        return;
    }
    if (volumes == &mRegions)
    {
        for (int i = 0; i < mRegions.size(); ++i)
            delete mRegions[i];
        mRegions.clear();
        mRegionTree.Clear();
        mRegionElemMap.clear();
        return;
    }
}

GameVolume* Navi::NewVolume(void* volumes)
{
    if (volumes == &mDoors)
    {
        static VolumeDoor door;
        door.id = 0;
        door.open = false;
        mDoors.push_back(door);
        return &mDoors.back();
    }
    if (volumes == &mRegions)
    {
        VolumeRegion* region = new VolumeRegion;
        region->id = 0;
        mRegions.push_back(region);
        return region;
    }
    return nullptr;
}

void Navi::ResizeLinkCount(void* volumes, void* volume, int count)
{
    if (volumes == &mRegions)
    {
        VolumeRegion* region = (VolumeRegion*)volume;
        region->links.resize(count);
    }
}

int* Navi::GetLinkPtr(void* volumes, void* volume, int index)
{
    if (volumes == &mRegions)
    {
        VolumeRegion* region = (VolumeRegion*)volume;
        if (index < 0 || index >= region->links.size())
            return nullptr;
        return &region->links.front() + index;
    }
    return nullptr;
}

void Navi::SetAABB(void* volumes, void* volume, float x, float y, float width, float height)
{
    if (volumes == &mRegions)
    {
        VolumeRegion* region = (VolumeRegion*)volume;
        auto* aabb = (Recast::AABB*)region->GetAABB();
        aabb->SetXY(x, y);
        aabb->SetSize(width, height);
    }
}

void Navi::AddQuadNode(void* volumes, void* volume, int deep)
{
    if (volumes == &mRegions)
    {
        VolumeRegion* region = (VolumeRegion*)volume;
        RegionElement* elem = mRegionTree.Add(region, deep);
        mRegionElemMap.insert(std::pair<int, RegionElement*>(region->id, elem));
    }
}

bool Navi::LoadVolumes(const char* path, void* volumes)
{
    if (!mNavQuery || !mNavMesh)
        return false;
    VolumesClear(volumes);
    const int maxSize = 1024;
    char buffer1[maxSize];
    char buffer2[maxSize];
    FILE* file = fopen(path, "r");
    if (!file)
        return false;
    char* buffer = buffer1;
    int bufferPos = 0;
    int readCount = 0;
    bool readEnd = false;
    
    GameVolume* volume = nullptr;
    
    const int volumeTagSize = strlen(sVolumeTag);
    const int nvertsTagSize = strlen(sNvertsTag);
    const int vertTagSize = strlen(sVertTag);
    const int nlinkTagSize = strlen(sNlinkTag);
    const int linkTagSize = strlen(sLinkTag);
    const int regionTreeTagSize = strlen(sRegionTreeTag);
    const int aabbTagSize = strlen(sAABBTag);
    const int treeDeepTagSize = strlen(sTreeDeepTag);
    int arrayIndex = 0;
    const int scanFormatSize = 256;
    char scanFormat[scanFormatSize];
    do
    {
        char* str = buffer == buffer1 ? buffer2 : buffer1;
        bool success = readLine(file, buffer, maxSize, bufferPos, readCount, str, readEnd);
        if (!success && readEnd)
            break;
        if (strncmp(str, sRegionTreeTag, regionTreeTagSize) == 0)
        {
            snprintf(scanFormat, scanFormatSize, "%sx=%%f,y=%%f,width=%%f,height=%%f", sRegionTreeTag);
            float x, y, width, height;
            sscanf(str, scanFormat, &x, &y, &width, &height);
            mRegionTree.Init(x, y, width, height);
            continue;
        }
        if (strncmp(str, sVolumeTag, volumeTagSize) == 0)
        {
            volume = NewVolume(volumes);
            snprintf(scanFormat, scanFormatSize, "%s%%d", sVolumeTag);
            sscanf(str, scanFormat, &volume->id);
            continue;
        }
        if (strncmp(str, sAABBTag, aabbTagSize) == 0)
        {
            snprintf(scanFormat, scanFormatSize, "%sx=%%f,y=%%f,width=%%f,height=%%f", sAABBTag);
            float x, y, width, height;
            sscanf(str, scanFormat, &x, &y, &width, &height);
            SetAABB(volumes, volume, x, y, width, height);
            continue;
        }
        if (strncmp(str, sTreeDeepTag, treeDeepTagSize) == 0)
        {
            snprintf(scanFormat, scanFormatSize, "%s%%d", sTreeDeepTag);
            int deep;
            sscanf(str, scanFormat, &deep);
            AddQuadNode(volumes, volume, deep);
            continue;
        }
        if (strncmp(str, sNvertsTag, nvertsTagSize) == 0)
        {
            snprintf(scanFormat, scanFormatSize, "%s%%d", sNvertsTag);
            int vertCount = 0;
            sscanf(str, scanFormat, &vertCount);
            volume->verts.resize(vertCount);
            arrayIndex = 0;
            continue;
        }
        if (strncmp(str, sVertTag, vertTagSize) == 0)
        {
            if (arrayIndex >= volume->verts.size())
                continue;
            snprintf(scanFormat, scanFormatSize, "%sx=%%f,y=%%f,z=%%f", sVertTag);
            Vector3& vert = volume->verts[arrayIndex++];
            sscanf(str, scanFormat, &vert.x, &vert.y, &vert.z);
            continue;
        }
        if (strncmp(str, sNlinkTag, nlinkTagSize) == 0)
        {
            snprintf(scanFormat, scanFormatSize, "%s%%d", sNlinkTag);
            int count = 0;
            sscanf(str, scanFormat, &count);
            ResizeLinkCount(volumes, volume, count);
            arrayIndex = 0;
            continue;
        }
        if (strncmp(str, sLinkTag, linkTagSize) == 0)
        {
            int* linkPtr = GetLinkPtr(volumes, volume, arrayIndex++);
            if (!linkPtr)
                continue;
            snprintf(scanFormat, scanFormatSize, "%s%%d", sLinkTag);
            sscanf(str, scanFormat, linkPtr);
            continue;
        }
    } while (true);
    fclose(file);
    return true;
}

bool Navi::LoadDoors(const char* path)
{
    if (!LoadVolumes(path, &mDoors))
        return false;
    
    for (int i = 0; i < mDoors.size(); ++i)
    {
        VolumeDoor* door = &mDoors[i];
        mDoorMap.insert(std::pair<int, VolumeDoor*>(door->id, door));
    }
    InitDoorsPoly();
    return true;
}

bool Navi::LoadRegions(const char* path)
{
    return LoadVolumes(path, &mRegions);
}

void Navi::InitDoorsPoly()
{
    if (!mNavQuery || !mNavMesh || mDoors.empty())
        return;
    for (int i = 0; i < mDoors.size(); ++i)
    {
        InitDoorPoly(mDoors[i]);
    }
}

void Navi::InitDoorPoly(VolumeDoor& door)
{
    if (!mNavMesh || !mNavQuery)
    {
        printf("Navi mesh or query is not inited\n");
        return;
    }
    if (door.verts.empty())
        return;
    float centerPos[3] = {0,0,0};
    const Vector3& firstVert = door.verts[0];
    float min[3] = {firstVert.x, firstVert.y, firstVert.z};
    float max[3] = {firstVert.x, firstVert.y, firstVert.z};
    for (int i = 0; i < door.verts.size(); ++i)
    {
        dtVadd(centerPos,centerPos,(float*)&door.verts[i]);
        dtVmin(min, (float*)&door.verts[i]);
        dtVmax(max, (float*)&door.verts[i]);
    }
    dtVscale(centerPos, centerPos, 1.0f/door.verts.size());
    float halfExtents[3];
    for (int i = 0; i < 3; ++i)
        halfExtents[i] = (max[i] - min[i]) * 0.5f;
    halfExtents[1] = 1.0f;
    dtQueryFilter filter;
    filter.setIncludeFlags(POLYFLAGS_DOOR);

    const int maxResult = 64;
    dtPolyRef resultRef[maxResult];
    memset(resultRef, 0, sizeof(dtPolyRef) * maxResult);
    int resultCount = 0;
    door.polyRefs.clear();
    dtStatus status = mNavQuery->queryPolygons(centerPos, halfExtents, &filter, resultRef, &resultCount, maxResult);
    if (!(status & DT_SUCCESS) || !resultCount)
        return;
    door.polyRefs.resize(resultCount);
    memcpy(&door.polyRefs.front(), resultRef, sizeof(dtPolyRef) * resultCount);
}

VolumeDoor* Navi::FindDoor(const int doorId)
{
    auto it = mDoorMap.find(doorId);
    if (it == mDoorMap.end())
        return nullptr;
    return it->second;
}

bool Navi::IsDoorOpen(VolumeDoor* door)
{
    if (!door)
    {
        printf("Cannot find door\n");
        return false;
    }
    return door->open;
}

void Navi::OpenDoor(VolumeDoor* door, const bool open)
{
    if (!door || !mNavMesh)
    {
        printf("Navi mesh or door is not inited\n");
        return;
    }
    if (door->open == open)
        return;
    door->open = open;
    OpenDoorPoly(door, open);
}

void Navi::OpenDoorPoly(VolumeDoor *door, const bool open)
{
    const dtMeshTile* tile = nullptr;
    const dtPoly* cpoly = nullptr;
    for (int i = 0; i < door->polyRefs.size(); ++i)
    {
        dtPolyRef polyRef = door->polyRefs[i];
        dtStatus status = mNavMesh->getTileAndPolyByRef(polyRef, &tile, &cpoly);
        if (status != DT_SUCCESS)
            continue;
        dtPoly* poly = (dtPoly*)cpoly;
        if (open)
            poly->flags &= ~POLYFLAGS_DOOR;
        else
            poly->flags |= POLYFLAGS_DOOR;
    }
}

dtStatus Navi::AddObstacle(const Vector3& pos, const float radius, const float height, dtObstacleRef* result)
{
    if (!mTileCache)
        return DT_FAILURE;
    return mTileCache->addObstacle((float*)&pos, radius, height, result);
}

dtStatus Navi::RemoveObstacle(const dtObstacleRef ref)
{
    if (!mTileCache)
        return DT_FAILURE;
    return mTileCache->removeObstacle(ref);
}

dtStatus Navi::RefreshObstacle()
{
    if (!mTileCache)
        return DT_FAILURE;
    bool finish = false;
    while (!finish)
    {
        int status = mTileCache->update(0, mNavMesh, &finish);
        if (!dtStatusSucceed(status))
            return DT_FAILURE;
    }
    return DT_SUCCESS;
}

bool Navi::IsPassable(const Vector3& start, const Vector3& end)
{
    if (!mRegionTree.IsInited())
    {
        printf("(Navi::IsPassable)Region is not inited\n");
        return false;
    }
    std::vector<VolumeRegion*> output;
    bool found = mRegionTree.Intersect(start.x, start.z, output, true);
    if (!found)
    {
        printf("(Navi::IsPassable)start is a invalid pos %f,%f,%f\n", start.x, start.y, start.z);
        return false;
    }
    VolumeRegion* startRegion = output[0];
    output.clear();
    found = mRegionTree.Intersect(end.x, end.z, output, true);
    if (!found)
    {
        printf("(Navi::IsPassable)end is a invalid pos %f,%f,%f\n", end.x, end.y, end.z);
        return false;
    }
    VolumeRegion* endRegion = output[0];
    if (startRegion == endRegion)
        return true;
    
    auto* endAABB = endRegion->GetAABB();
    float endMidX = endAABB->GetLeft() + endAABB->GetWidth() * 0.5f;
    float endMidY = endAABB->GetBottom() + endAABB->GetHeight() * 0.5f;
    
    struct RegionBlock
    {
        const VolumeRegion* mRegion;
        float mCost;
        float mRemain;
        int mPre;
        
        const VolumeRegion* GetRegion() const
        {
            return mRegion;
        }
        
        float GetCost() const
        {
            return mCost;
        }
        
        float GetTotalCost() const
        {
            return mCost + mRemain;
        }
        
        int GetPre() const
        {
            return mPre;
        }
        
        void Set(const VolumeRegion* region, float cost, float remain, int pre)
        {
            mRegion = region;
            mCost = cost;
            mRemain = remain;
            mPre = pre;
        }
    };
    
    std::vector<RegionBlock> memory;
    std::list<int> openList;
    std::vector<int> closeList;
    
    auto insert2OpenList = [&](int blockIndex)
    {
        RegionBlock& block = memory[blockIndex];
        float cost = block.GetTotalCost();
        for (auto it = openList.begin(); it != openList.end(); ++it)
        {
            RegionBlock& openBlock = memory[*it];
            if (cost < openBlock.GetTotalCost())
            {
                openList.insert(it, blockIndex);
                return;
            }
        }
        openList.push_back(blockIndex);
    };
    
    auto stepRegion = [&](int pre, const RegionBlock& openBlock)
    {
        const VolumeRegion* region = openBlock.mRegion;
        auto* aabb = region->GetAABB();
        float midX = aabb->GetLeft() + aabb->GetWidth() * 0.5f;
        float midY = aabb->GetBottom() + aabb->GetHeight() * 0.5f;
        for (int i = 0; i < region->links.size(); ++i)
        {
            int linkId = region->links[i];
            int doorId = getLinkDoorId(linkId);
            if (doorId > 0)
            {
                const VolumeDoor* door = FindDoor(doorId);
                if (!door)
                {
                    printf("Cannot find doorId=%d by region=%d", doorId, region->id);
                    continue;
                }
                if (!door->open)
                    continue;
            }
            int regionId = getLinkVolumeId(linkId);
            const VolumeRegion* linkRegion = FindRegion(regionId);
            if (!linkRegion)
            {
                printf("Cannot find regionId=%d by region=%d", regionId, region->id);
                continue;
            }
            auto* linkAABB = linkRegion->GetAABB();
            float linkMidX = linkAABB->GetLeft() + linkAABB->GetWidth() * 0.5f;
            float linkMidY = linkAABB->GetBottom() + linkAABB->GetHeight() * 0.5f;
            float diffX = linkMidX - midX;
            float diffY = linkMidY - midY;
            float cost = diffX * diffX + diffY * diffY + openBlock.GetCost();
            diffX = endMidX - linkMidX;
            diffY = endMidY - linkMidY;
            float remain = diffX * diffX + diffY * diffY;
            int blockIndex = memory.size();
            memory.push_back(RegionBlock());
            RegionBlock* block = &memory.back();
            block->Set(linkRegion, cost, remain, pre);
            if (linkRegion == endRegion)
            {
                closeList.push_back(blockIndex);
                openList.clear();
                return true;
            }
            insert2OpenList(blockIndex);
        }
        return false;
    };
    
    memory.push_back(RegionBlock());
    RegionBlock* block = &memory.back();
    block->Set(startRegion, 0, 0, -1);
    openList.push_back(0);
    
    found = false;
    while (!openList.empty())
    {
        int openIndex = openList.front();
        RegionBlock& openBlock = memory[openIndex];
        openList.pop_front();
        int pre = closeList.size();
        closeList.push_back(openIndex);
        found = stepRegion(pre, openBlock);
        if (found)
            break;
    }
    
    return found;
}

int Navi::FindPath(const Vector3& start, const Vector3& end, const Vector3& polySize)
{
    if (!mNavMesh || !mNavQuery)
    {
        printf("Navi mesh or query is not inited\n");
        return DT_FAILURE;
    }
    
    if (!IsPassable(start, end))
    {
        printf("(Navi::FindPath)Region is not passable\n");
        return DT_FAILURE;
    }
    
    dtStatus status = DT_SUCCESS;
    dtPolyRef startRef = 0;
    dtPolyRef endRef = 0;

    status = mNavQuery->findNearestPoly((float*)&start, (float*)&polySize, mFilter, &startRef, nullptr);
    if (!(status & DT_SUCCESS))
    {
        printf("Cannot find start poly (%f, %f, %f)\n", start.x, start.y, start.z);
        return status;
    }
    
    status = mNavQuery->findNearestPoly((float*)&end, (float*)&polySize, mFilter, &endRef, nullptr);
    if (!(status & DT_SUCCESS))
    {
        printf("Cannot find end poly (%f, %f, %f)\n", end.x, end.y, end.z);
        return status;
    }
    
    mSearchedPolyCount = 0;
    status = mNavQuery->findPath(startRef, endRef, (float*)&start, (float*)&end, mFilter, mSearchPolys, &mSearchedPolyCount, MAX_POLYS);
    if (!(status & DT_SUCCESS))
    {
        printf("Cannot find path from start(%f, %f, %f) to end(%f, %f, %f)\n", start.x, start.y, start.z, end.x, end.y, end.z);
        return status;
    }
    mPathCount = 0;
    if (mSearchedPolyCount)
    {
        // In case of partial path, make sure the end point is clamped to the last polygon.
        float epos[3];
        dtVcopy(epos, (float*)&end);
        if (mSearchPolys[mSearchedPolyCount - 1] != endRef)
            mNavQuery->closestPointOnPoly(mSearchPolys[mSearchedPolyCount - 1], (float*)&end, epos, nullptr);
        
        mNavQuery->findStraightPath((float*)&start, epos, mSearchPolys, mSearchedPolyCount,
                                     (float*)mPath, nullptr, nullptr, &mPathCount, MAX_POLYS, 0);
    }
    if (mPathCount < 2)
        return DT_FAILURE;
    return status;
}
