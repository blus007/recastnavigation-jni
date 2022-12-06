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
};

struct VolumeDoor
{
    int id;
    std::vector<Vector3> verts;
    std::vector<dtPolyRef> polyRefs;
};

class Navi
{
    class dtNavMesh* mNavMesh;
    class dtNavMeshQuery* mNavQuery;
    class dtTileCache* mTileCache;
    
    struct LinearAllocator* mAlloc;
    struct FastLZCompressor* mComp;
    struct MeshProcess* mProc;
    
    std::vector<VolumeDoor> mDoors;
    
    void InitDoorPoly(VolumeDoor& door);
    VolumeDoor* FindDoor(const int doorId);
    bool IsDoorOpen(VolumeDoor* door);
    void OpenDoor(VolumeDoor* door, const bool open);
    
public:
    Navi();
    ~Navi();
    
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
    void OpenAllDoors(const bool open);
};
