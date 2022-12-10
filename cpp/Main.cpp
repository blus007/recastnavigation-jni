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
    printf("load door success = %d\n", success);
    
    Vector3 start(54.9729767f, -2.37854576f, 4.9592514f);
    Vector3 end(49.1615448f, -2.33363724f, 18.9671612f);
    
//    (dtPolyRef) [0] = 281475014459398
//    (dtPolyRef) [1] = 281475014459393
//    (dtPolyRef) [2] = 281475014459394
//    (dtPolyRef) [3] = 281475014459395
//    (dtPolyRef) [4] = 281475030188036
//    (dtPolyRef) [5] = 281475030188038
//    (dtPolyRef) [6] = 281475030188037
//    54.9729767,-2.37854576,4.9592514
//    56.010685,-2.26951694,6.79984951
//    56.010685,-2.0695169,9.79984951
//    49.1106834,-2.26951671,13.3998508
//    49.1106834,-2.26951671,16.3998508
//    49.1615448,-2.33363724,18.9671612
    navi.OpenDoor(1, false);
    success = navi.FindPath(start, end);
    printf("find path close door success = %d\n", success);
    
//    (dtPolyRef) [0] = 281475014459398
//    (dtPolyRef) [1] = 281475014459393
//    (dtPolyRef) [2] = 281475014459394
//    (dtPolyRef) [3] = 281475014459395
//    (dtPolyRef) [4] = 281475030188036
//    (dtPolyRef) [5] = 281475030188032
//    (dtPolyRef) [6] = 281475030188037
//    54.9729767,-2.37854576,4.9592514
//    56.010685,-2.26951694,6.79984951
//    56.010685,-2.0695169,9.79984951
//    52.1106834,-2.06951666,13.3998508
//    49.1615448,-2.33363724,18.9671612
    navi.OpenDoor(1, true);
    success = navi.FindPath(start, end);
    printf("find path open door success = %d\n", success);
}
