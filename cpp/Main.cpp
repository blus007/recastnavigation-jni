#include <stdio.h>
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
    Navi navi;
    int success = navi.LoadMesh(RECAST_BIN"/Output/nav_test_obs_navi.bin");
    printf("load navi success = %d\n", success);
    success = navi.LoadDoors(RECAST_BIN"/Output/nav_test.door");
    printf("load door success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    success = navi.LoadRegions(RECAST_BIN"/Output/nav_test.region");
    printf("load region success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    
    Vector3 start(54.9729767f, -2.37854576f, 4.9592514f);
    Vector3 end(49.1615448f, -2.33363724f, 18.9671612f);
    const Vector3* path;
    
//    54.9729767,-2.37854576,4.9592514
//    56.010685,-2.26951694,6.79984951
//    56.010685,-2.0695169,9.79984951
//    49.1106834,-2.26951671,13.3998508
//    49.1106834,-2.26951671,16.3998508
//    49.1615448,-2.33363724,18.9671612
    navi.OpenDoor(1, false);
    success = navi.FindPath(start, end);
    printf("find path close door success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    printf("find path count = %d\n", navi.GetPathCount());
    path = navi.GetPath();
    for (int i = 0; i < navi.GetPathCount(); ++i) {
        printf("pos = (%f,%f,%f)\n", path[i].x, path[i].y, path[i].z);
    }
    
//    54.9729767,-2.37854576,4.9592514
//    56.010685,-2.26951694,6.79984951
//    56.010685,-2.0695169,9.79984951
//    52.1106834,-2.06951666,13.3998508
//    49.1615448,-2.33363724,18.9671612
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
    success = navi.FindPath(start, end);
    printf("find path open door add obstacle success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    printf("find path count = %d\n", navi.GetPathCount());
    path = navi.GetPath();
    for (int i = 0; i < navi.GetPathCount(); ++i) {
        printf("pos = (%f,%f,%f)\n", path[i].x, path[i].y, path[i].z);
    }
}
