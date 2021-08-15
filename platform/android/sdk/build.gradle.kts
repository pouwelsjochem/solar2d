plugins {
    id("com.android.library")
}

android {
    ndkVersion = "18.1.5063045"
    compileSdkVersion(30)

    defaultConfig {
        minSdkVersion(15)
        targetSdkVersion(30)
        versionCode = 1
        versionName = "1.0"
    }

    sourceSets["main"].manifest.srcFile(file("AndroidManifest-New.xml"))
    sourceSets["main"].java.srcDirs(file("src"), file("../../../external/JNLua/src/main"))
    sourceSets["main"].java.filter.exclude("**/script/**")
    sourceSets["main"].res.srcDirs(file("res-new"))

    externalNativeBuild {
        cmake {
            path = file("../sdk/CMakeLists.txt")
        }
    }
    useLibrary("org.apache.http.legacy")

    libraryVariants.all {
        generateBuildConfigProvider!!.configure {
            enabled = false
        }
    }
    testOptions {
        testVariants.all {
            generateBuildConfigProvider!!.configure {
                enabled = false
            }
        }
    }
}

dependencies {
    api(files("../../../plugins/build-core/network/android/network.jar"))
}
