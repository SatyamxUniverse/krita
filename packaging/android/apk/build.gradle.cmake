buildscript {
    repositories {
        google()
        jcenter()
    }

    dependencies {
        classpath 'com.android.tools.build:gradle:4.1.0'
    }
}

repositories {
    google()
    jcenter()
}

apply plugin: 'com.android.application'

dependencies {
    implementation fileTree(dir: 'libs', include: ['*.jar', '*.aar'])
}

ext {
    abi = System.getenv('ANDROID_ABI') ?: project.ext.properties['abi']
    ndkpath = System.getenv('ANDROID_NDK_HOME') ?: project.ext.properties['ndk.dir']  // TODO: Newer versions of Qt do support this dir
    installPrefix = '../krita-android-build'
    qtResPath = System.getenv('KRITA_BUILD_APPBUNDLE') ? '../i/src/android/java' : ""
}

task configure() {
    doLast {
        if (abi == null && !project.hasProperty("abi")) {
            logger.error('ANDROID_ABI not specified using the default one instead: arm64-v8a')
            abi = 'arm64-v8a'
        }

        def libs = new File(installPrefix, 'lib')
        if (!libs.exists()) {
            throw new GradleException('Krita libraries not found, please check if -p=krita-bin finished without errors')
        }
    }
}

task writeConfig(type: WriteProperties) {
    outputFile = file("gradle.properties")
    def gradleProperties = new Properties()
    outputFile.withInputStream { gradleProperties.load(it) }

    properties gradleProperties
    property 'abi', abi
    property 'ndk.dir', ndkpath
}

// copy libs(plugins) which start with krita*.so and rename
// them to start with `lib_`
task copyLibs(type: Copy) {
    from "$installPrefix/lib"
    into "libs/$abi"
    rename ('^krita(.*).so$', 'lib_krita$1.so')
}

/*
 * androiddeployqt doesn't fully copy the directories. Hidden directories
 * to be specific. That's why we copy manually and then delete the partial
 * one it creates
 */
task copyAssets(type: Copy) {
    from "$installPrefix/share/"
    into 'assets/'
    include '**'
}

/*
 * Remove "share" folder in assets. It is copied both manually and by
 * androiddeployqt(reason in copyAssets task).
 */
task removeDuplicateAssets(type: Delete) {
    delete "assets/share"
}

def isPackagingAPK() {
    return gradle.taskGraph.hasTask(assembleRelease) || gradle.taskGraph.hasTask(assembleDebug)
}

android {
    compileSdkVersion androidCompileSdkVersion.toInteger()

    ndkPath ndkpath

    sourceSets {
        main {
            manifest.srcFile 'AndroidManifest.xml'
            java.srcDirs = [qt5AndroidDir + '/src', qtResPath + '/src', 'src', 'java']
            aidl.srcDirs = [qt5AndroidDir + '/src', qtResPath + '/src', 'src', 'aidl']
            res.srcDirs = [qt5AndroidDir + '/res',  qtResPath + '/res', 'res']
            resources.srcDirs = ['src']
            renderscript.srcDirs = ['src']
            assets.srcDirs = ['assets']
            jniLibs.srcDirs = ['libs', 'lib']
       }
    }

    // This is needed because, gradle by default ignores hidden assets.
    aaptOptions {
        ignoreAssetsPattern "!.foajasoie"
    }

    lintOptions {
        abortOnError false
    }

    project.ext.constant = 5

    def versionMajor    = @KRITA_STABLE_VERSION_MAJOR@
    def versionMinor    = @KRITA_STABLE_VERSION_MINOR@

    /**
     * This version does **not** correspond to the patch version
     * of Krita. Instead, it is just incremented for every public
     * (including alpha and beta) release of versionMajor.versionMinor
     * branch
     */
    def versionRelease  = @KRITA_ANDROID_RELEASE_VERSION@

    defaultConfig {
        targetSdkVersion 30
        minSdkVersion 23
        versionCode project.ext.constant * 1000000 + versionMajor * 10000 + versionMinor * 100 + versionRelease
        versionName "@KRITA_ANDROID_VERSION_STRING@"
        archivesBaseName = "krita-$abi-$versionName"
    }

    preBuild.dependsOn(configure)
    configure.onlyIf { isPackagingAPK() }
    configure.finalizedBy(writeConfig)
    configure.finalizedBy(copyLibs)
    configure.finalizedBy(copyAssets)
    configure.finalizedBy(removeDuplicateAssets)
}

dependencies {
    implementation 'com.android.billingclient:billing:3.0.3'
}

