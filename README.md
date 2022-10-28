# OculusSdk-moonlight

## How to build

Tested with android studio 3.5.3 on linux:
https://redirector.gvt1.com/edgedl/android/studio/ide-zips/3.5.3.0/android-studio-ide-191.6010548-linux.tar.gz

Install ndk 21.0.6113669 with android SDK manager.

Rerun android studio with the following environment variable set:
```
export ANDROID_NDK_HOME=~/Android/Sdk/ndk/21.0.6113669
export ANDROID_HOME=~/Android/Sdk
```

Open project OculusSdk-moonlight/VrSamples/Native/CinemaSDK/Projects/Android and let android studio download all dependencies.

You should end with the following installed:
* sdk platforms: api 33, 28, 27, 26, 25, 21
* sdk build tools: 28.0.3, 33.0.0
* android sdk platform tools: 33.0.3
