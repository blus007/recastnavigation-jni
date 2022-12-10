#include <stdio.h>
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "DetourTileCache.h"
#include "Navi.h"

int main(int argc, char* argv[])
{
    printf("Main\n");
    printf("%s", RECAST_BIN"/Output/nav_test_obs_navi.bin\n");
    Navi navi;
    int success = navi.LoadMesh(RECAST_BIN"/Output/nav_test_obs_navi.bin");
    printf("load navi success = %d\n", success);
    success = navi.LoadDoors(RECAST_BIN"/Output/nav_test.volume");
    printf("load door success = %s\n", dtStatusSucceed(success) ? "success" : "fail");
    
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
