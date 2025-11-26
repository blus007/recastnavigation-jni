#include <stdio.h>
#include <string.h>
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourTileCache.h"
#include "Navi.h"
#include "Circle.h"

struct TestStruct
{
    Recast::AABB aabb;
    Recast::Circle circle;
    
    void CalcAABB()
    {
        aabb.SetXY(circle.GetX() - circle.GetRadius(), circle.GetY() - circle.GetRadius());
        float size = circle.GetRadius() * 2;
        aabb.SetSize(size, size);
    }
    
    Recast::Circle* GetCircle()
    {
        return &circle;
    }
    
    const Recast::Circle* GetCircle() const
    {
        return &circle;
    }
    
    const Recast::AABB* GetAABB() const
    {
        return &aabb;
    }
    
    bool IsContain(float x, float y) const
    {
        return circle.IsContain(x, y);
    }
    
    bool Intersect(const Recast::Circle& other) const
    {
        return circle.Intersect(other);
    }
};


int main(int argc, char* argv[])
{
    printf("Main\n");
    {
        Recast::QuadTree<VolumeRegion>* tree = new Recast::QuadTree<VolumeRegion>(0, 0, 1000, 1000);
        VolumeRegion rs[100];
        Recast::QuadTree<VolumeRegion>::Element* el[100];
        for (int i = 0; i < 10; ++i)
        {
            for (int j = 0; j < 10; ++j)
            {
                int index = i * 10 + j;
                VolumeRegion* r = &rs[index];
                r->aabb.SetXY(j * 100, i * 100);
                r->aabb.SetSize(100, 100);
                auto* elem = tree->Add(r);
                el[index] = elem;
            }
        }
//        rs[99].aabb.SetXY(0, 0);
//        tree->Refresh(el[99]);
        std::vector<VolumeRegion*> output;
        tree->Intersect(1, 1, output);
        delete tree;
    }
    {
        Recast::QuadTree<TestStruct>* tree = new Recast::QuadTree<TestStruct>(0, 0, 1000, 1000, 8);
        TestStruct ts[100];
        Recast::QuadTree<TestStruct>::Element* el[100];
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                int index = i * 4 + j;
                TestStruct* t = &ts[index];
                t->GetCircle()->SetXY(100, 100);
                t->GetCircle()->SetRadius(100);
                t->CalcAABB();
                auto* elem = tree->Add(t);
                el[index] = elem;
            }
        }
        ts[15].GetCircle()->SetRadius(10);
        ts[15].CalcAABB();
        tree->Refresh(el[15]);
        Recast::Circle circle(300, 100, 110);
        std::vector<TestStruct*> output;
        tree->Intersect(circle, output);
        delete tree;
    }
    
    printf("%s", RECAST_BIN"/Output/nav_test_obs_navi.bin\n");
    Navi navi(MAX_SEARCH_POLYS);
    int success = navi.LoadMesh(RECAST_BIN"/Output/nav_test_obs_navi.bin", MAX_SEARCH_POLYS);
    printf("load navi success = %s\n", success ? "success" : "fail");
    success = navi.LoadDoors(RECAST_BIN"/Output/nav_test.door");
    printf("load door success = %s\n", success ? "success" : "fail");
    success = navi.LoadRegions(RECAST_BIN"/Output/nav_test.region");
    printf("load region success = %s\n", success ? "success" : "fail");
    
    Vector3 start(51.1867447f, -1.80287552f, -24.2468395f);
    Vector3 end(40.9090157f, -1.78335953f, -25.2743912f);
    const Vector3* path;
    
//    51.1867447, -1.80287552, -24.2468395f
//    47.3106842, -2.26951671, -25.9001503
//    44.3106842, -1.86951673, -25.9001503
//    40.9090157, -1.78335953, -25.2743912
    navi.OpenDoor(1, false);
    success = navi.FindPath(start, end);
    printf("find path close door success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    printf("find path count = %d\n", navi.GetPathCount());
    path = navi.GetPath();
    for (int i = 0; i < navi.GetPathCount(); ++i) {
        printf("pos = (%f,%f,%f)\n", path[i].x, path[i].y, path[i].z);
    }
    
//    51.1867447, -1.80287552, -24.2468395f
//    40.9090157, -1.78335953, -25.2743912
    navi.OpenDoor(1, true);
    success = navi.FindPath(start, end);
    printf("find path open door success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    printf("find path count = %d\n", navi.GetPathCount());
    path = navi.GetPath();
    for (int i = 0; i < navi.GetPathCount(); ++i) {
        printf("pos = (%f,%f,%f)\n", path[i].x, path[i].y, path[i].z);
    }
    
//    obstacle
    navi.CloseAllDoorsPoly();
    Vector3 obstacle(53.742393f, -2.262424f, 12.016539f);
    dtObstacleRef ref = 0;
    success = navi.AddObstacle(obstacle, 1.0f, 2.0f, &ref);
    navi.RefreshObstacle();
    printf("add obstacle success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    navi.InitDoorsPoly();
    navi.RecoverAllDoorsPoly();
//    pos = (54.972977,-2.378546,4.959251)
//    pos = (56.010685,-2.269517,6.799850)
//    pos = (56.010685,-2.069517,9.799850)
//    pos = (54.510685,-1.869517,13.099851)
//    pos = (49.161545,-2.333637,18.967161)
    start.Set(54.972977, -2.378546, 4.959251);
    end.Set(49.161545, -2.333637, 18.967161);
    success = navi.FindPath(start, end);
    printf("find path open door add obstacle success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    printf("find path count = %d\n", navi.GetPathCount());
    path = navi.GetPath();
    for (int i = 0; i < navi.GetPathCount(); ++i) {
        printf("pos = (%f,%f,%f)\n", path[i].x, path[i].y, path[i].z);
    }
}
