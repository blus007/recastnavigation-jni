#include <stdio.h>
#include <string>
#include <list>
#include <set>
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "Recast.h"
#include "fastlz.h"
#include "Filelist.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "Util.h"
#include "Navi.h"

// check if is 64bit
COMPILE_TEST(BIT_SIZE, sizeof(void*) == 8);
COMPILE_TEST(LONG_LONG_SIZE, sizeof(long long) == 8);
COMPILE_TEST(POLY_REF_SIZE, sizeof(dtPolyRef) == 8);

const int TILECACHESET_MAGIC = 'W'<<24 | 'L'<<16 | 'R'<<8 | 'D';
const int TILECACHESET_VERSION = 1;

/////////////////////////////////////////////////////////////////
// GameVolume
void GameVolume::CalcAABB()
{
    const int vertSize = (int)verts.size();
    aabb.SetXY(0, 0);
    aabb.SetSize(0, 0);
    if (vertSize <= 0)
        return;
    float minX = HUGE_VALF;
    float maxX = -HUGE_VALF;
    float minZ = HUGE_VALF;
    float maxZ = -HUGE_VALF;
    for (int i = 0; i < vertSize; ++i)
    {
        const Vector3& vert = verts[i];
        minX = minX < vert.x ? minX : vert.x;
        maxX = maxX > vert.x ? maxX : vert.x;
        minZ = minZ < vert.z ? minZ : vert.z;
        maxZ = maxZ > vert.z ? maxZ : vert.z;
    }
    aabb.SetXY(minX, minZ);
    aabb.SetSize(maxX - minX, maxZ - minZ);
}

// verts order is counter clockwise
bool GameVolume::IsContain(float x, float z) const
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
        if (result < 0)
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
Navi::Navi(int maxPolys, int maxObstacles)
:mNavMesh(nullptr)
,mNavQuery(nullptr)
,mTileCache(nullptr)
,mSearchedPolyCount(0)
,mPathCount(0)
,mDefaultPolySize(0, 6, 0)
,mMaxPolys(maxPolys)
,mMaxObstacles(maxObstacles)
{
    mAlloc = new LinearAllocator(32000);
    mComp = new FastLZCompressor;
    mProc = new MeshProcess;
    
    mPathFilter = new dtQueryFilter;
    mPathFilter->setIncludeFlags(POLYFLAGS_WALK);
    mPathFilter->setExcludeFlags(~((unsigned short)POLYFLAGS_WALK));

    mPolyFilter = new dtQueryFilter;
    mPolyFilter->setIncludeFlags(POLYFLAGS_ALL);
    
    mSearchPolys = new dtPolyRef[mMaxPolys];
    mPath = new Vector3[mMaxPolys];
    mPathPolys = new dtPolyRef[mMaxPolys];
    mPathRemoves = new bool[mMaxPolys];
}

Navi::~Navi()
{
    dtFreeNavMeshQuery(mNavQuery);
    dtFreeNavMesh(mNavMesh);
    dtFreeTileCache(mTileCache);
    
    delete mPath;
    delete mSearchPolys;
    delete mPathFilter;
    delete mPolyFilter;
    
    delete mAlloc;
    delete mComp;
    delete mProc;
    
    for (int i = 0; i < mRegions.size(); ++i)
        delete mRegions[i];
}

int Navi::GetPathFilterInclude()
{
    return (unsigned int)mPathFilter->getIncludeFlags();
}

int Navi::GetPathFilterExclude()
{
    return (unsigned int)mPathFilter->getExcludeFlags();
}

void Navi::SetPathFilter(int include, int exclude)
{
    mPathFilter->setIncludeFlags(include);
    mPathFilter->setExcludeFlags(exclude);
}

int Navi::GetPolyFilterInclude()
{
    return (unsigned int)mPolyFilter->getIncludeFlags();
}

int Navi::GetPolyFilterExclude()
{
    return (unsigned int)mPolyFilter->getExcludeFlags();
}

void Navi::SetPolyFilter(int include, int exclude)
{
    mPolyFilter->setIncludeFlags(include);
    mPolyFilter->setExcludeFlags(exclude);
}

bool Navi::LoadMesh(const char* path, const int maxSearchNodes)
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
    if (mMaxObstacles >= 0)
        header.cacheParams.maxObstacles = mMaxObstacles;
    if (header.cacheParams.maxObstacles < 1)
        header.cacheParams.maxObstacles = 1;
    
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
    status = mNavQuery->init(mNavMesh, maxSearchNodes);
    if (dtStatusFailed(status))
    {
        LOG_ERROR("Load Mesh failed by mNavQuery->init");
        return false;
    }
    return true;
}

bool Navi::LoadDoors(const char* path)
{
    if (!LoadDoorsInternal(path))
        return false;
    InitDoorsPoly();
    
    float minX = HUGE_VALF;
    float maxX = -HUGE_VALF;
    float minZ = HUGE_VALF;
    float maxZ = -HUGE_VALF;
    for (int i = 0; i < mDoors.size(); ++i)
    {
        VolumeDoor* door = &mDoors[i];
        mDoorMap.insert(std::pair<int, VolumeDoor*>(door->id, door));

        float left = door->aabb.GetLeft();
        float right = door->aabb.GetRight();
        float bottom = door->aabb.GetBottom();
        float top = door->aabb.GetTop();
        minX = minX < left ? minX : left;
        maxX = maxX > right ? maxX : right;
        minZ = minZ < bottom ? minZ : bottom;
        maxZ = maxZ > top ? maxZ : top;
    }
    mDoorTree.Init(minX, minZ, maxX - minX, maxZ - minZ);
    const int doorSize = (int)mDoors.size();
    for (int i = 0; i < doorSize; ++i)
    {
        VolumeDoor& door = mDoors[i];
        DoorElement* elem = mDoorTree.Add(&door, 0);
        mDoorElemMap.insert(std::pair<int, DoorElement*>(door.id, elem));
    }
    InitProvinceLink();

    return true;
}

bool Navi::LoadDoorsInternal(const char* path)
{
    if (!mNavQuery || !mNavMesh)
        return false;
    ClearDoors();
    try
    {
        std::ifstream read(path);
        if (!read.is_open())
            return false;
        nlohmann::json data = nlohmann::json::parse(read);
        const int volumeCount = data["volumes"].size();
        for (int i = 0; i < volumeCount; ++i)
        {
            mDoors.push_back(VolumeDoor());
            auto& door = mDoors.back();
            auto& volume = data["volumes"][i];

            door.id = volume["id"];
            const int vertCount = volume["verts"].size();
            door.verts.resize(vertCount);
            for (int j = 0; j < vertCount; ++j)
            {
                auto& jvert = volume["verts"][j];
                Vector3& vert = door.verts[j];
                vert.x = jvert[0];
                vert.y = jvert[1];
                vert.z = jvert[2];
            }
            auto& links = volume["link"];
            door.links[0] = links[0];
            door.links[1] = links[1];
            door.open = false;
            door.CalcAABB();
        }
    }
    catch (const std::exception& e)
    {
        ClearDoors();
        return false;
    }
    return true;
}

void Navi::InitProvinceLink()
{
    mProvinceLinkMap.clear();
    for (int i = 0; i < mDoors.size(); ++i)
    {
        const VolumeDoor& door = mDoors[i];
        for (int i = 0; i < 2; ++i)
        {
            int fromProvince = door.links[i];
            int toProvince = door.links[(i + 1) % 2];
            ProvinceLinkMap::iterator linkIt = mProvinceLinkMap.find(fromProvince);
            if (linkIt == mProvinceLinkMap.end())
            {
                auto pair = mProvinceLinkMap.emplace(std::pair<int, ProvinceDoorMap>(fromProvince, ProvinceDoorMap()));
                linkIt = pair.first;
            }
            ProvinceDoorMap& doorMap = linkIt->second;
            ProvinceDoorMap::iterator doorIt = doorMap.find(toProvince);
            if (doorIt == doorMap.end())
            {
                auto pair = doorMap.emplace(std::pair<int, DoorList>(toProvince, DoorList()));
                doorIt = pair.first;
            }
            DoorList& doors = doorIt->second;
            doors.push_back(door.id);
        }
    }
}

void Navi::ClearDoors()
{
    mDoors.clear();
    mDoorMap.clear();
    mDoorTree.Clear();
    mDoorElemMap.clear();
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
        LOG_ERROR("Navi mesh or query is not inited");
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
    {
        LOG_ERROR("InitDoorPoly failed by mNavQuery->queryPolygons");
        return;
    }
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
        LOG_ERROR("Cannot find door");
        return false;
    }
    return door->open;
}

void Navi::OpenDoor(VolumeDoor* door, const bool open)
{
    if (!door || !mNavMesh)
    {
        LOG_ERROR("Navi mesh or door is not inited");
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

bool Navi::LoadRegions(const char* path)
{
    return LoadRegionsInternal(path);
}

bool Navi::LoadRegionsInternal(const char* path)
{
    if (!mNavQuery || !mNavMesh)
        return false;
    ClearRegions();
    try
    {
        std::ifstream read(path);
        if (!read.is_open())
            return false;
        nlohmann::json data = nlohmann::json::parse(read);
        auto& info = data["info"];
        mRegionTree.Init(info["x"], info["z"], info["width"], info["height"]);
        const int volumeCount = data["volumes"].size();
        for (int i = 0; i < volumeCount; ++i)
        {
            VolumeRegion* regionPtr = new VolumeRegion;
            mRegions.push_back(regionPtr);

            auto& region = *regionPtr;
            auto& volume = data["volumes"][i];

            region.id = volume["id"];
            region.province = volume["province"];
            const int vertCount = volume["verts"].size();
            region.verts.resize(vertCount);
            for (int j = 0; j < vertCount; ++j)
            {
                auto& jvert = volume["verts"][j];
                Vector3& vert = region.verts[j];
                vert.x = jvert[0];
                vert.y = jvert[1];
                vert.z = jvert[2];
            }
            auto& aabb = volume["aabb"];
            region.aabb.SetXY(aabb["x"], aabb["z"]);
            region.aabb.SetSize(aabb["width"], aabb["height"]);
            RegionElement* elem = mRegionTree.Add(regionPtr, volume["deep"]);
            mRegionElemMap.insert(std::pair<int, RegionElement*>(region.id, elem));
        }
    }
    catch (const std::exception& e)
    {
        ClearRegions();
        return false;
    }
    return true;
}

void Navi::ClearRegions()
{
    for (int i = 0; i < mRegions.size(); ++i)
        delete mRegions[i];
    mRegions.clear();
    mRegionTree.Clear();
    mRegionElemMap.clear();
}

bool Navi::PointInRegion(float x, float z, const VolumeRegion& region)
{
    if (!mRegionTree.IsInited())
    {
        LOG_ERROR("(Navi::PointInRegion)Region is not inited");
        return false;
    }

    // find region
    std::vector<VolumeRegion*> output;
    bool found = mRegionTree.Intersect(x, z, output, true);
    if (!found)
        return false;
    VolumeRegion* ptRegion = output[0];
    int province = ptRegion->province;
    return region.province == province;
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

bool Navi::FindProvince(const Vector3& pos, std::vector<int>& provinces)
{
    std::vector<VolumeRegion*> output;
    std::vector<VolumeDoor*> outputDoor;
    bool found = mRegionTree.Intersect(pos.x, pos.z, output, true);
    if (found)
    {
        VolumeRegion* region = output[0];
        provinces.push_back(region->province);
        return true;
    }
    found = mDoorTree.Intersect(pos.x, pos.z, outputDoor, true);
    if (!found)
        return false;
    VolumeDoor* door = outputDoor[0];
    provinces.push_back(door->links[0]);
    provinces.push_back(door->links[1]);
    return true;
}

bool Navi::IsProvincePassable(int startProvince, int endProvince)
{
    if (startProvince == endProvince)
        return true;
    std::set<int> addProvince;
    std::list<int> openProvince;
    openProvince.push_back(startProvince);
    addProvince.emplace(startProvince);
    bool found = false;
    while (!found && !openProvince.empty())
    {
        int fromProvince = openProvince.front();
        openProvince.pop_front();
        ProvinceLinkMap::iterator linkIt = mProvinceLinkMap.find(fromProvince);
        if (linkIt == mProvinceLinkMap.end())
            continue;
        ProvinceDoorMap& doorMap = linkIt->second;
        for (ProvinceDoorMap::iterator doorIt = doorMap.begin(); doorIt != doorMap.end(); ++doorIt)
        {
            int toProvince = doorIt->first;
            if (addProvince.find(toProvince) != addProvince.end())
                continue;
            DoorList& doors = doorIt->second;
            VolumeDoor* openDoor = nullptr;
            for (DoorList::iterator doorIt = doors.begin(); doorIt != doors.end(); ++doorIt)
            {
                int doorId = *doorIt;
                VolumeDoor* door = FindDoor(doorId);
                if (!door)
                    continue;
                if (door->open)
                {
                    openDoor = door;
                    break;
                }
            }
            if (openDoor)
            {
                if (toProvince == endProvince)
                {
                    found = true;
                    break;
                }
                openProvince.push_back(toProvince);
                addProvince.emplace(toProvince);
            }
        }
    }

    return found;
}

bool Navi::IsPassable(const Vector3& start, const Vector3& end)
{
    if (!mRegionTree.IsInited())
    {
        LOG_ERROR("(Navi::IsPassable)Region is not inited");
        return false;
    }
    std::vector<int> startProvinces;
    if (!FindProvince(start, startProvinces))
    {
        LOG_ERROR("(Navi::IsPassable)start is a invalid pos %f,%f,%f", start.x, start.y, start.z);
        return false;
    }
    std::vector<int> endProvinces;
    if (!FindProvince(end, endProvinces))
    {
        LOG_ERROR("(Navi::IsPassable)end is a invalid pos %f,%f,%f", end.x, end.y, end.z);
        return false;
    }
    // check province connected
    for (int i = 0; i < startProvinces.size(); ++i)
    {
        int startProvince = startProvinces[i];
        for (int j = 0; j < endProvinces.size(); ++j)
        {
            int endProvince = endProvinces[j];
            if (IsProvincePassable(startProvince, endProvince))
                return true;
        }
    }
    return false;
}

bool Navi::WalkablePoly(const dtPolyRef polyRef)
{
    const dtMeshTile* tile = nullptr;
    const dtPoly* poly = nullptr;
    dtStatus status = mNavMesh->getTileAndPolyByRef(polyRef, &tile, &poly);
    if (status != DT_SUCCESS)
        return false;
    return poly->flags == POLYFLAGS_WALK;
}

void Navi::MakePathOutOfBlock(const Vector3& polySize)
{
    if (mPathCount < 2)
        return;
    int lastIndex = mPathCount - 1;
    for (int i = lastIndex; i > 0; --i)
    {
        Vector3& fromPt = mPath[lastIndex];
        Vector3& toPt = mPath[lastIndex - 1];
        Vector3 diffPt(toPt);
        diffPt.Sub(fromPt);
        float length = diffPt.Length();
        const int splitCount = (int)(length / 0.1f);
        diffPt.Mul(1.0f / (float)splitCount);
        Vector3 pt(fromPt);
        for (int j = 0; j < splitCount; ++j)
        {
            dtPolyRef polyRef = 0;
            dtStatus status = mNavQuery->findNearestPoly((float*)&pt, (float*)&polySize, mPolyFilter, &polyRef, nullptr);
            if (status & DT_SUCCESS)
            {
                if (WalkablePoly(polyRef))
                {
                    mPath[i] = pt;
                    mPathCount = i + 1;
                    return;
                }
            }
            pt.Add(diffPt);
        }
    }
}

void Navi::StraightenPath()
{
    if (mPathCount <= 2)
        return;
    bool* removes = mPathRemoves;
    memset(removes, 0, sizeof(bool) * mPathCount);
    int removeCount = 0;
    int maxFrom = mPathCount - 2;
    float* path = (float*)mPath;
    for (int i = 0; i < maxFrom; ++i)
    {
        dtPolyRef fromRef = mPathPolys[i];
        int j = i + 2;
        for (; j < mPathCount; ++j)
        {
            float t = 0;
            dtStatus rayStatus = mNavQuery->raycast(fromRef, path + i * 3, path + j * 3, mPathFilter,
                &t, nullptr, mSearchPolys, &mSearchedPolyCount, mMaxPolys);
            if (dtStatusSucceed(rayStatus) && t > 1)
            {
                removes[j - 1] = true;
                ++removeCount;
                continue;
            }
            break;
        }
        i = j - 2;
    }
    if (!removeCount)
        return;
    int count = mPathCount - removeCount;
    int src = 0;
    for (int i = 0; i < count; ++i, ++src)
    {
        for (; src < mPathCount; ++src)
        {
            if (!removes[src])
                break;
        }
        if (i == src)
            continue;
        memcpy(path + i * 3, path + src * 3, sizeof(float) * 3);
        mPathPolys[i] = mPathPolys[src];
    }
    mPathCount = count;
}

int Navi::FindPath(const Vector3& start, const Vector3& end, const Vector3& polySize)
{
    if (!mNavMesh || !mNavQuery)
    {
        LOG_ERROR("Navi mesh or query is not inited");
        return DT_FAILURE;
    }
    
    if (!IsPassable(start, end))
    {
        LOG_ERROR("(Navi::FindPath)Region is not passable");
        return DT_FAILURE;
    }
    
    dtStatus status = DT_SUCCESS;
    dtPolyRef startRef = 0;
    dtPolyRef endRef = 0;

    status = mNavQuery->findNearestPoly((float*)&start, (float*)&polySize, mPolyFilter, &startRef, nullptr);
    if (!(status & DT_SUCCESS))
    {
        LOG_ERROR("Cannot find start poly (%f, %f, %f)", start.x, start.y, start.z);
        return status;
    }
    
    status = mNavQuery->findNearestPoly((float*)&end, (float*)&polySize, mPolyFilter, &endRef, nullptr);
    if (!(status & DT_SUCCESS))
    {
        LOG_ERROR("Cannot find end poly (%f, %f, %f)", end.x, end.y, end.z);
        return status;
    }

    bool startWalkable = WalkablePoly(startRef);
    bool endWalkable = WalkablePoly(endRef);
    if (!startWalkable && !endWalkable)
    {
        LOG_INFO("Cannot walk from block to block start(%f, %f, %f) => end(%f, %f, %f)", start.x, start.y, start.z, end.x, end.y, end.z);
        return DT_FAILURE;
    }
    float* startPtr = (float*)&start;
    float* endPtr = (float*)&end;
    bool exchanged = false;
    if (!startWalkable)
    {
        exchanged = true;

        dtPolyRef temp = startRef;
        startRef = endRef;
        endRef = temp;
        
        float* tempPtr = startPtr;
        startPtr = endPtr;
        endPtr = tempPtr;
    }
    
    mSearchedPolyCount = 0;
    status = mNavQuery->findPath(startRef, endRef, startPtr, endPtr, mPathFilter, mSearchPolys, &mSearchedPolyCount, mMaxPolys);
    if (!(status & DT_SUCCESS))
    {
        LOG_ERROR("Cannot find path from start(%f, %f, %f) to end(%f, %f, %f)", start.x, start.y, start.z, end.x, end.y, end.z);
        return status;
    }
    mPathCount = 0;
    if (mSearchedPolyCount)
    {
        // In case of partial path, make sure the end point is clamped to the last polygon.
        float epos[3];
        dtVcopy(epos, endPtr);
        if (mSearchPolys[mSearchedPolyCount - 1] != endRef)
            mNavQuery->closestPointOnPoly(mSearchPolys[mSearchedPolyCount - 1], endPtr, epos, nullptr);
        
        mNavQuery->findStraightPath(startPtr, epos, mSearchPolys, mSearchedPolyCount,
                                     (float*)mPath, nullptr, mPathPolys, &mPathCount, mMaxPolys, 0);
        StraightenPath();
        MakePathOutOfBlock(polySize);
        if (mPathCount > 1 && exchanged)
        {
            const int vectorSize = sizeof(float) * 3;
            float* pathPtr = (float*)mPath;
            int mid = mPathCount / 2;
            int lastIndex = mPathCount - 1;
            for (int i = 0; i < mid; ++i)
            {
                int exchangePos = lastIndex - i;
                memcpy(epos, pathPtr + i * 3, vectorSize);
                memcpy(pathPtr + i * 3, pathPtr + exchangePos * 3, vectorSize);
                memcpy(pathPtr + exchangePos * 3, epos, vectorSize);
            }
        }
    }
    if (mPathCount < 2)
        return DT_FAILURE;
    return status;
}
