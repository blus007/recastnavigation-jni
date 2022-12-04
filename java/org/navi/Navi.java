package org.navi;

public class Navi {

    public native String loadMesh(String filePath);

    static {
        System.loadLibrary("RecastJni");
    }

    public static void main(String[] args) {
        Navi navi = new Navi();
        String ret = navi.loadMesh("aaa/bbb");
        System.out.println(ret);
    }
}