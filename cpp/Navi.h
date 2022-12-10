#pragma once

#include <vector>

enum PolyAreas
{
    POLYAREA_GROUND,
    POLYAREA_WATER,
    POLYAREA_ROAD,
    POLYAREA_DOOR,
    POLYAREA_GRASS,
    POLYAREA_JUMP
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

struct VolumeDoor
{
    int id;
    std::vector<Vector3> verts;
    std::vector<dtPolyRef> polyRefs;
    bool open;
};

class Navi
{
    class dtNavMesh* mNavMesh;
    class dtNavMeshQuery* mNavQuery;
    class dtTileCache* mTileCache;
    
    struct LinearAllocator* mAlloc;
    struct FastLZCompressor* mComp;
    struct MeshProcess* mProc;
    
    class dtQueryFilter* mFilter;
    dtPolyRef* mSearchPolys;
    int mSearchedPolyCount;
    Vector3* mPath;
    int mPathCount;
    
    Vector3 mDefaultPolySize;
    
    std::vector<VolumeDoor> mDoors;
    
    void InitDoorPoly(VolumeDoor& door);
    VolumeDoor* FindDoor(const int doorId);
    bool IsDoorOpen(VolumeDoor* door);
    void OpenDoor(VolumeDoor* door, const bool open);
    void OpenDoorPoly(VolumeDoor* door, const bool open);
    
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
    int FindPath(const Vector3& start, const Vector3& end, const Vector3& polySize);
    inline int FindPath(const Vector3& start, const Vector3& end)
    {
        return FindPath(start, end, mDefaultPolySize);
    }
    inline const int GetPathCount() { return mPathCount; }
    inline const Vector3* GetPath() { return mPath; }
};
