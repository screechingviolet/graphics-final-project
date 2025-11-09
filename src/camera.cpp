#include <stdexcept>
#include "camera.h"

// glm::mat4 Camera::getViewMatrix() const {
//     // Optional TODO: implement the getter or make your own design
//     return viewMatrix;
// }

// float Camera::getAspectRatio() const {
//     // Optional TODO: implement the getter or make your own design
//     return (float)width/(float)height;
// }

float Camera::getHeightAngle() const {
    // Optional TODO: implement the getter or make your own design
    return heightAngle;
}

float Camera::getFocalLength() const {
    // Optional TODO: implement the getter or make your own design
    return focalLength;
}

float Camera::getAperture() const {
    // Optional TODO: implement the getter or make your own design
    return aperture;
}
