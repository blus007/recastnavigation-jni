package com.test;

import org.navi.Navi;

public class Main {
    public static void main(String[] args) {
        Navi navi = new Navi();
        navi.init();

        boolean success = navi.loadMesh("../thirdparty/recastnavigation/RecastDemo/Bin/Output/nav_test_obs_navi.bin");
        System.out.println(String.format("load mesh %s", success ? "success" : "fail"));
        success = navi.loadDoors("../thirdparty/recastnavigation/RecastDemo/Bin/Output/nav_test.volume");
        System.out.println(String.format("load doors %s", success ? "success" : "fail"));

        System.out.println(String.format("getMaxObstacleReqCount %d", navi.getMaxObstacleReqCount()));
        System.out.println(String.format("getAddedObstacleReqCount %d", navi.getAddedObstacleReqCount()));
        System.out.println(String.format("getObstacleReqRemainCount %d", navi.getObstacleReqRemainCount()));

        // Vector3 start(54.9729767f, -2.37854576f, 4.9592514f);
        // Vector3 end(49.1615448f, -2.33363724f, 18.9671612f);
        navi.openDoor(1, false);
        int status = navi.findPath(54.9729767f, -2.37854576f, 4.9592514f, 49.1615448f, -2.33363724f, 18.9671612f, 2, 4, 2);
        success = Navi.isSuccess(status);
        System.out.println(String.format("find path close door success = %s", success ? "success" : "fail"));
        if (success) {
            final int posSize = navi.getPosSize();
            System.out.println(String.format("find path count = %d", posSize));
            final float[] posArray = navi.getPosArray();
            for (int i = 0; i < posSize; ++i) {
                System.out.println(String.format("pos = (%f,%f,%f)", 
                    posArray[i * 3], posArray[i * 3 + 1], posArray[i * 3 + 2]));
            }
        }

        navi.openDoor(1, true);
        status = navi.findPath(54.9729767f, -2.37854576f, 4.9592514f, 49.1615448f, -2.33363724f, 18.9671612f, 2, 4, 2);
        success = Navi.isSuccess(status);
        System.out.println(String.format("find path open door success = %s", success ? "success" : "fail"));
        if (success) {
            final int posSize = navi.getPosSize();
            System.out.println(String.format("find path count = %d", posSize));
            final float[] posArray = navi.getPosArray();
            for (int i = 0; i < posSize; ++i) {
                System.out.println(String.format("pos = (%f,%f,%f)", 
                    posArray[i * 3], posArray[i * 3 + 1], posArray[i * 3 + 2]));
            }
        }

        //    obstacle
        navi.closeAllDoorsPoly();
        int ref = navi.addObstacle(53.742393f, -2.262424f, 12.016539f, 1.0f, 2.0f);
        navi.refreshObstacle();
        System.out.println(String.format("add obstacle success = %s\n", ref != 0 ? "success" : "fail"));
        navi.initDoorsPoly();
        navi.recoverAllDoorsPoly();
        // find path pass by obstacle
        status = navi.findPath(54.9729767f, -2.37854576f, 4.9592514f, 49.1615448f, -2.33363724f, 18.9671612f, 2, 4, 2);
        success = Navi.isSuccess(status);
        System.out.println(String.format("find path open door add obstacle success = %s", success ? "success" : "fail"));
        if (success) {
            final int posSize = navi.getPosSize();
            System.out.println(String.format("find path count = %d", posSize));
            final float[] posArray = navi.getPosArray();
            for (int i = 0; i < posSize; ++i) {
                System.out.println(String.format("pos = (%f,%f,%f)", 
                    posArray[i * 3], posArray[i * 3 + 1], posArray[i * 3 + 2]));
            }
        }

        navi.destroy();
    }
}