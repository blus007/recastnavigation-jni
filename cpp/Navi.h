#pragma once

#include <vector>
#include <map>
#include "QuadTree.h"

#ifdef _WIN32
#   ifdef EXPORT_DLL
#       define NAVI_API __declspec(dllexport)
#   else
#       define NAVI_API __declspec(dllimport)
#   endif
#else
#   define NAVI_API
#endif

enum PolyAreas
{
    POLYAREA_GROUND,
    POLYAREA_WATER,
    POLYAREA_ROAD,
    POLYAREA_DOOR,
    POLYAREA_GRASS,
    POLYAREA_JUMP,
    POLYAREA_REGION,
};

enum PolyFlags
{
    POLYFLAGS_WALK          = 0x01,        // Ability to walk (ground, grass, road)
    POLYFLAGS_SWIM          = 0x02,        // Ability to swim (water).
    POLYFLAGS_DOOR          = 0x04,        // Ability to move through doors.
    POLYFLAGS_JUMP          = 0x08,        // Ability to jump.
    POLYFLAGS_DISABLED      = 0x10,        // Disabled polygon
    POLYFLAGS_ALL           = 0xffff    // All abilities.
};

extern const int MAX_POLYS;

struct Vector3
{
    float x;
    float y;
    float z;
    
    Vector3()
    :x(0)
    ,y(0)
    ,z(0)
    {}
    
    Vector3(float inX, float inY, float inZ)
    :x(inX)
    ,y(inY)
    ,z(inZ)
    {}
};

struct NAVI_API GameVolume
{
    int id;
    Recast::AABB aabb;
    std::vector<Vector3> verts;

    void CalcAABB();
    bool IsContain(float x, float y) const;

    const Recast::AABB* GetAABB() const
    {
        return &aabb;
    }
};

struct NAVI_API VolumeDoor : public GameVolume
{
    std::vector<dtPolyRef> polyRefs;
    int links[2];
    bool open;
};

struct NAVI_API VolumeRegion : public GameVolume
{
    int province;
};

inline int buildLinkId(int volumeId, int doorId)
{
    return volumeId | (doorId << 16);
}

inline int getLinkVolumeId(int linkId)
{
    return linkId & 0x0000ffff;
}

inline int getLinkDoorId(int linkId)
{
    return (linkId >> 16) & 0x0000ffff;
}

typedef Recast::QuadTree<VolumeDoor> DoorTree;
typedef Recast::QuadTree<VolumeDoor>::Element DoorElement;
typedef Recast::QuadTree<VolumeRegion> RegionTree;
typedef Recast::QuadTree<VolumeRegion>::Element RegionElement;
typedef std::vector<int> DoorList;
// province => doors
typedef std::map<int, DoorList> ProvinceDoorMap;
// province => neighbor province
typedef std::map<int, ProvinceDoorMap> ProvinceLinkMap;

class NAVI_API Navi
{
    class dtNavMesh* mNavMesh;
    class dtNavMeshQuery* mNavQuery;
    class dtTileCache* mTileCache;
    
    struct LinearAllocator* mAlloc;
    struct FastLZCompressor* mComp;
    struct MeshProcess* mProc;
    
    class dtQueryFilter* mPathFilter;
    class dtQueryFilter* mPolyFilter;
    dtPolyRef* mSearchPolys;
    int mSearchedPolyCount;
    Vector3* mPath;
    int mPathCount;
    
    Vector3 mDefaultPolySize;
    
    std::vector<VolumeDoor> mDoors;
    std::map<int, VolumeDoor*> mDoorMap;
    DoorTree mDoorTree;
    std::map<int, DoorElement*> mDoorElemMap;
    
    std::vector<VolumeRegion*> mRegions;
    RegionTree mRegionTree;
    std::map<int, RegionElement*> mRegionElemMap;

    ProvinceLinkMap mProvinceLinkMap;
    
    void InitProvinceLink();
    bool LoadDoorsInternal(const char* path);
    void ClearDoors();
    bool LoadRegionsInternal(const char* path);
    void ClearRegions();
    
    void InitDoorPoly(VolumeDoor& door);
    VolumeDoor* FindDoor(const int doorId);
    bool IsDoorOpen(VolumeDoor* door);
    void OpenDoor(VolumeDoor* door, const bool open);
    void OpenDoorPoly(VolumeDoor* door, const bool open);
    bool PointInRegion(float x, float z, const VolumeRegion& region);
    bool IsProvincePassable(int startProvince, int endProvince);
    bool FindProvince(const Vector3& pos, std::vector<int>& provinces);
    bool WalkablePoly(const dtPolyRef polyRef);
    
public:
    Navi();
    ~Navi();
    
    inline void SetDefaultPolySize(float x, float y, float z)
    {
        mDefaultPolySize.x = x;
        mDefaultPolySize.y = y;
        mDefaultPolySize.z = z;
    }
    
    bool LoadMesh(const char* path);
    bool LoadDoors(const char* path);
    bool LoadRegions(const char* path);
    
    inline RegionElement* FindRegionElem(int id)
    {
        auto it = mRegionElemMap.find(id);
        if (it == mRegionElemMap.end())
            return nullptr;
        return it->second;
    }
    inline VolumeRegion* FindRegion(int id)
    {
        RegionElement* elem = FindRegionElem(id);
        if (!elem)
            return nullptr;
        return elem->GetValue();
    }
    inline int GetRegionId(const Vector3& pos)
    {
        if (!mRegionTree.IsInited())
            return 0;
        std::vector<VolumeRegion*> output;
        bool found = mRegionTree.Intersect(pos.x, pos.z, output, true);
        if (!found)
            return 0;
        VolumeRegion* region = output[0];
        return region->id;
    }
    void InitDoorsPoly();
    inline bool IsDoorExist(const int doorId)
    {
        VolumeDoor* door = FindDoor(doorId);
        return !!door;
    }
    bool IsDoorOpen(const int doorId)
    {
        VolumeDoor* door = FindDoor(doorId);
        return IsDoorOpen(door);
    }
    inline void OpenDoor(const int doorId, const bool open)
    {
        VolumeDoor* door = FindDoor(doorId);
        OpenDoor(door, open);
    }
    inline void OpenAllDoors(const bool open)
    {
        for (int i = 0; i < mDoors.size(); ++i)
        {
            OpenDoor(&mDoors[i], open);
        }
    }
    inline void CloseAllDoorsPoly()
    {
        for (int i = 0; i < mDoors.size(); ++i)
        {
            OpenDoorPoly(&mDoors[i], false);
        }
    }
    inline void RecoverAllDoorsPoly()
    {
        for (int i = 0; i < mDoors.size(); ++i)
        {
            if (!mDoors[i].open)
                continue;
            OpenDoorPoly(&mDoors[i], mDoors[i].open);
        }
    }
    dtStatus AddObstacle(const Vector3& pos, const float radius, const float height, dtObstacleRef* result);
    dtStatus RemoveObstacle(const dtObstacleRef ref);
    dtStatus RefreshObstacle();
    inline int GetMaxObstacleReqCount()
    {
        if (!mTileCache)
            return 0;
        return mTileCache->getMaxObstacleReqCount();
    }
    inline int GetAddedObstacleReqCount()
    {
        if (!mTileCache)
            return 0;
        return mTileCache->getAddedObstacleReqCount();
    }
    inline int GetObstacleReqRemainCount()
    {
        if (!mTileCache)
            return 0;
        return mTileCache->getObstacleReqRemainCount();
    }
    bool IsPassable(const Vector3& start, const Vector3& end);
    int FindPath(const Vector3& start, const Vector3& end, const Vector3& polySize);
    inline int FindPath(const Vector3& start, const Vector3& end)
    {
        return FindPath(start, end, mDefaultPolySize);
    }
    inline const int GetPathCount() { return mPathCount; }
    inline const Vector3* GetPath() { return mPath; }
};
