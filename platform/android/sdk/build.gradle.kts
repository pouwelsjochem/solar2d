plugins {
    id("com.android.library")
}

val buildDirectory = layout.buildDirectory.asFile.get()

android {
    namespace = "com.ansca.corona"
    ndkVersion = "18.1.5063045"
    compileSdk = 35

    defaultConfig {
        minSdk = 15
        version = 1
    }
    sourceSets["main"].manifest.srcFile(file("AndroidManifest-New.xml"))
    sourceSets["main"].java.srcDirs(file("src"), file("../../../external/JNLua/src/main"), file("../../../plugins/network/android/src"))
    sourceSets["main"].java.filter.exclude("**/script/**")
    sourceSets["main"].res.srcDirs(file("res-new"))

    externalNativeBuild {
        cmake {
            path = file("../sdk/CMakeLists.txt")
        }
    }
    useLibrary("org.apache.http.legacy")

}

val networkHelperOutputDir = file("$buildDirectory/generated/source/networkJava")
val networkHelper = tasks.register<Copy>("generateNetworkHelper") {
    from("../../../plugins/network/android/src/")
    include("**/LuaHelper.java.template")

    into(networkHelperOutputDir)
    rename { it.removeSuffix(".template") }

    val networkLua = file("../../../plugins/network/shared/network.lua")
    val luaCode = networkLua.readText()
            .replace("lib.", "network.")
            .replace("\\", "\\\\")
            .replace("\"", "\\\"")
            .lines()
            .joinToString("\\n\" +\n\"","\"", "\";")
    this.inputs.file(networkLua)
    expand(mutableMapOf("luaCode" to luaCode))
}
android.libraryVariants.all {
    registerJavaGeneratingTask(networkHelper, networkHelperOutputDir)
}
