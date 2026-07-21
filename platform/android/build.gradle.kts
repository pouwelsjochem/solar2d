buildscript {
    repositories {
        google()
        jcenter()
        mavenCentral()
    }
    dependencies {
        classpath("org.jetbrains.kotlin:kotlin-gradle-plugin:1.6.21")
        classpath("com.android.tools.build:gradle:8.8.0")
        classpath("com.beust:klaxon:5.5")
    }
}

val coronaResourcesDir = providers.gradleProperty("coronaResourcesDir").orNull
val windows = System.getProperty("os.name").lowercase().contains("windows")
val linux = System.getProperty("os.name").lowercase().contains("linux")
val buildToolsDirCandidate = providers.gradleProperty("solar2DBuildToolsDir").orNull ?: run {
    val candidates = when {
        windows -> listOfNotNull(coronaResourcesDir?.let { "$it/../Native" }, System.getenv("CORONA_PATH")?.let { "$it/Native" })
        linux -> listOfNotNull(coronaResourcesDir?.let { "$it/Native" })
        else -> listOfNotNull(
                coronaResourcesDir?.let { "$it/../../../Native" },
                coronaResourcesDir?.let { "$it/../../../../../.." })
    }
    candidates.firstOrNull { file("$it/Corona").isDirectory } ?: candidates.firstOrNull() ?: "$rootDir/Native"
}
val solar2DBuildToolsDir = file(buildToolsDirCandidate).canonicalPath
extra["solar2DBuildToolsDir"] = solar2DBuildToolsDir

allprojects {
    repositories {
        google()
        jcenter()
        mavenCentral()
        // maven(url = "https:// some custom repo")
        flatDir {
            dirs("$solar2DBuildToolsDir/Corona/android/lib/gradle", "$solar2DBuildToolsDir/Corona/android/lib/Corona/libs")
        }
    }
}

tasks.register<Delete>("clean") {
    delete(rootProject.layout.buildDirectory.asFile.get())
}
