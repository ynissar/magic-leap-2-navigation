#define ALOG_TAG "com.magicleap.capi.sample.depth_camera"

#include "confidence_material.h"
#include "depth_material.h"

#include <time.h>
#include <cstdlib>
#include <future>

#include <app_framework/application.h>
#include <app_framework/components/renderable_component.h>
#include <app_framework/geometry/quad_mesh.h>
#include <app_framework/gui.h>
#include <app_framework/material/textured_material.h>
#include <app_framework/toolset.h>

#include <ml_depth_camera.h>
#include <ml_head_tracking.h>
#include <ml_perception.h>
#include <ml_time.h>
#include <marker_detection.h>

#ifdef ML_LUMIN
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#endif

using namespace ml::app_framework;
using namespace std::chrono_literals;

class ToolDefinition {
    public:
    static 

    private:

    vector<float> detect_marker_positions(float* depth_camera_buffer, int width, int height) {
        // 1. get the pixel values of the pixels with AHAT intensity above a threshold
    }

    vector<float> connected_component_detection(float* depth_camera_buffer, int width, int height) {
        // area estimation of the connected components
    }

    vector<float> calculate_marker_positions(vector<float> connected_component_areas) {
        // calculate the positions of the markers
    }

};