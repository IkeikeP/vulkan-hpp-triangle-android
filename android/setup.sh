#!/bin/bash

# Include the shared scripts and utility methods that are common to all platforms.
. ../shared-scripts.sh

# We will assume that the Android SDK is in the default location that Android Studio installs it to.
ANDROID_SDK="/Users/$USER/Library/Android/sdk"
echo "Using Android SDK at: $ANDROID_SDK"

# We will assume that we'll use Java that is bundled with Android Studio.
export JAVA_HOME="/Applications/Android Studio.app/Contents/jre/jdk/Contents/Home"

# We will be using a specific version of the Android NDK.
NDK_VERSION="20.0.5594570"
ANDROID_NDK="$ANDROID_SDK/ndk/$NDK_VERSION"
echo "Using Android NDK at: $ANDROID_NDK"

# This command will automatically answer 'yes' to each of the licences that a user normally has to manually agree to.
echo "Auto accepting Android SDK licenses ..."
yes | $ANDROID_SDK/tools/bin/sdkmanager --licenses

# This is the collection of Android SDK components that our code base will need.
echo "Installing required Android SDK components ..."
$ANDROID_SDK/tools/bin/sdkmanager \
    "platform-tools" \
    "build-tools;28.0.3" \
    "tools" \
    "platforms;android-28" \
    "cmake;3.10.2.4988404" \
    "ndk;$NDK_VERSION"
