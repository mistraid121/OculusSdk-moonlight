# OculusSdk-moonlight

## How to build

Tested with android studio 3.0 on linux:
https://redirector.gvt1.com/edgedl/android/studio/ide-zips/3.0.0.18/android-studio-ide-171.4408382-linux.zip

Install ndk 17.2.4988734 with android SDK manager.

Rerun android studio with the following environment variable set:
```
export ANDROID_NDK_HOME=~/Android/Sdk/ndk/17.2.4988734/
export ANDROID_HOME=~/Android/Sdk
```

Open project OculusSdk-moonlight/VrSamples/Native/CinemaSDK/Projects/Android and let android studio download all dependencies.

You should end with the following installed:
* sdk platforms: api 33, 28, 27, 25, 21
* sdk build tools: 26.0.2, 28.0.3, 33.0.0
* android sdk platform tools: 33.0.3
* android sdk tools: 26.1.1
