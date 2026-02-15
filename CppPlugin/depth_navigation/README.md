# Depth Camera Sample App

This sample demonstrates how to setup depth camera and stream live feed to a view surface.


## Prerequisites

Refer to https://developer.magicleap.cloud/learn/docs/guides/native/capi-getting-started

## Gui
  - If not all required permissions are granted, a simple dialog will be shown instead of a surface

## Running on device

```sh
adb install ./app/build/outputs/apk/ml2/debug/com.magicleap.capi.sample.depth_camera-debug.apk
adb shell am start -a android.intent.action.MAIN -n com.magicleap.capi.sample.depth_camera/android.app.NativeActivity
```

## Removing from device

```sh
adb uninstall com.magicleap.capi.sample.depth_camera
```

## What to Expect

 - Surface with depth camera feeds should be visible.
 - Pixels on a feed symbolize distance from camera to the nearest obstruction.
 - Over each preview a color legend bar should be visible.
 - User can manually choose which streams to request from the API via the checkboxes on the GUI.
 - User can re-request the permissions, if by mistake the user did not grant them at the first try.