#include <stdio.h>
#include "DetourCommon.h"
#include "DetourNavMesh.h"
#include "Navi.h"

int main(int argc, char* argv[])
{
    printf("Main\n");
    printf("%s", RECAST_BIN"/Output/nav_test_obs_navi.bin\n");
    Navi navi;
    int success = navi.LoadMesh(RECAST_BIN"/Output/nav_test_obs_navi.bin");
    printf("load navi success = %d\n", success);
    success = navi.LoadDoors(RECAST_BIN"/Output/nav_test.volume");
    printf("load door success = %d\n", success);
    navi.InitDoorsPoly();
    
    Vector3 start(54.9729767f, -2.37854576f, 4.9592514f);
    Vector3 end(49.1615448f, -2.33363724f, 18.9671612f);
    
//    (dtPolyRef) [0] = 281475014459398
//    (dtPolyRef) [1] = 281475014459393
//    (dtPolyRef) [2] = 281475014459394
//    (dtPolyRef) [3] = 281475014459395
//    (dtPolyRef) [4] = 281475030188036
//    (dtPolyRef) [5] = 281475030188038
//    (dtPolyRef) [6] = 281475030188037
    navi.OpenDoor(1, false);
    success = navi.FindPath(start, end);
    printf("find path close door success = %d\n", success);
    
//    (dtPolyRef) [0] = 281475014459398
//    (dtPolyRef) [1] = 281475014459393
//    (dtPolyRef) [2] = 281475014459394
//    (dtPolyRef) [4] = 281475030188036
//    (dtPolyRef) [5] = 281475030188032
//    (dtPolyRef) [6] = 281475030188037
    navi.OpenDoor(1, true);
    success = navi.FindPath(start, end);
    printf("find path open door success = %d\n", success);
}
