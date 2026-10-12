// Copyright 2023-2026 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "OrbitCam.h"

#include <moonray/rendering/rndr/RenderContext.h>

using namespace scene_rdl2::math;

namespace {

// Print out matrix in lua format so it can be pasted into an rdla file.
void printMatrix(const char *comment, const Mat4f &m)
{
    std::cout << "-- " << comment << "\n"
    << "[\"node xform\"] = Mat4("
    << m.vx.x << ", " << m.vx.y << ", " << m.vx.z << ", " << m.vx.w << ", "
    << m.vy.x << ", " << m.vy.y << ", " << m.vy.z << ", " << m.vy.w << ", "
    << m.vz.x << ", " << m.vz.y << ", " << m.vz.z << ", " << m.vz.w << ", "
    << m.vw.x << ", " << m.vw.y << ", " << m.vw.z << ", " << m.vw.w << "),\n"
    << std::endl;
}

}   // end of anon namespace

namespace moonray_gui_v2 {

// Bitflags that represent current movement state
enum
{
    ORBIT_FORWARD     = 0x0001,
    ORBIT_BACKWARD    = 0x0002,
    ORBIT_LEFT        = 0x0004,
    ORBIT_RIGHT       = 0x0008,
    ORBIT_UP          = 0x0010,
    ORBIT_DOWN        = 0x0020,
    ORBIT_SLOW_DOWN   = 0x0040,
    ORBIT_SPEED_UP    = 0x0080
};

// orbit camera (taken from embree sample code)
// This camera is in world space
struct Camera {

    Camera () :
        mPosition(0.0f, 0.0f, -3.0f),
        mViewDir(normalize(-mPosition)),
        mUp(0.0f, 1.0f, 0.0f),
        mFocusDistance(1.0f) {}

    Xform3f camera2world () const {
        // Warning: this needs to be double precision.  If we use single then
        // there is slight imprecision introduced when computing the cross products
        // when orthonormalizing the vectors.
        // This normally wouldn't be a problem, but this camera2world matrix
        // gets fed into OrbitCam::resetTransform() when the scene is reloaded.
        // OrbitCam::resetTransform() then sets the vectors used for camera2world,
        // but those came from camera2world.  Thus camera2world is used to set
        // itself, and the old value might be identical to the new if the user
        // hasn't manipulated the camera.
        // The imprecision from the single-precision cross products causes
        // a slight difference in camera2world when there should be no change
        // at all when camera2world hasn't changed.  This causes nondeterminism
        // between successive renders in moonray_gui as this has a slight effect
        // on the ray directions each time.
        const Vec3d vz = -mViewDir;
        const Vec3d vx = normalize(cross(Vec3d(mUp), vz));
        const Vec3d vy = normalize(cross(vz, vx));
        return Xform3f(
            static_cast<float>(vx.x), static_cast<float>(vx.y),
            static_cast<float>(vx.z), static_cast<float>(vy.x),
            static_cast<float>(vy.y), static_cast<float>(vy.z),
            static_cast<float>(vz.x), static_cast<float>(vz.y),
            static_cast<float>(vz.z), mPosition.x, mPosition.y, mPosition.z);
    }

    Xform3f world2camera () const { return camera2world().inverse();}

    Vec3f world2camera(const Vec3f& p) const { return transformPoint(world2camera(),p);}
    Vec3f camera2world(const Vec3f& p) const { return transformPoint(camera2world(),p);}

    void translate (float dx, float dy, float dz)
    {
        const float moveSpeed = 0.03f;
        dx *= -moveSpeed;
        dy *= moveSpeed;
        dz *= moveSpeed;
        Xform3f xfm = camera2world();
        const Vec3f ds = transformVector(xfm, Vec3f(dx,dy,dz));
        mPosition += ds;
    }

    void rotate (const float dtheta, const float dphi)
    {
        const float rotateSpeed = 0.005f;
        // in camera local space, viewDir is always (0, 0, -1)
        // and its spherical coordinate is always (PI, 0)
        const float theta = sPi - dtheta * rotateSpeed;
        const float phi   = -dphi * rotateSpeed;

        float cosPhi, sinPhi;
        sincos(phi, &sinPhi, &cosPhi);
        float cosTheta, sinTheta;
        sincos(theta, &sinTheta, &cosTheta);

        const float x = cosPhi*sinTheta;
        const float y = sinPhi;
        const float z = cosPhi*cosTheta;

        mViewDir = transformVector(camera2world(), Vec3f(x, y, z));
    }

    void rotateOrbit (const float dtheta, const float dphi)
    {
        bool currentlyValid = false;
        if (scene_rdl2::math::abs(dot(mUp, mViewDir)) < 0.999f) {
            currentlyValid = true;
        }

        const float rotateSpeed = 0.005f;
        // in camera local space, viewDir is always (0, 0, -1)
        // and its spherical coordinate is always (PI, 0)
        const float theta = sPi - dtheta * rotateSpeed;
        const float phi   = -dphi * rotateSpeed;

        float cosPhi, sinPhi;
        sincos(phi, &sinPhi, &cosPhi);
        float cosTheta, sinTheta;
        sincos(theta, &sinTheta, &cosTheta);

        const float x = cosPhi*sinTheta;
        const float y = sinPhi;
        const float z = cosPhi*cosTheta;

        Vec3f newViewDir = transformVector(camera2world(),Vec3f(x,y,z));
        Vec3f newPosition = mPosition + mFocusDistance * (mViewDir - newViewDir);

        // Don't update 'position' if dir is near parallel with the up vector
        // unless the current state of 'position' is already invalid.
        if (scene_rdl2::math::abs(dot(mUp, newViewDir)) < 0.999f || !currentlyValid) {
            mPosition = newPosition;
            mViewDir = newViewDir;
        }
    }

    void dolly (const float ds)
    {
        const float dollySpeed = 0.005f;
        const float k = scene_rdl2::math::pow((1.0f - dollySpeed), ds);
        const Vec3f focusPoint = mPosition + mViewDir * mFocusDistance;
        mPosition += mFocusDistance * (1-k) * mViewDir;
        mFocusDistance = length(focusPoint - mPosition);
    }


    void roll (const float ds)
    {
        const float rollSpeed = 0.005f;
        const Vec3f& axis = mViewDir;
        mUp = transform3x3(Mat4f::rotate(Vec4f(axis[0], axis[1], axis[2], 0.0f), -ds * rollSpeed), mUp);
    }

public:
    Vec3f mPosition;   // position of camera
    Vec3f mViewDir;    // lookat direction
    Vec3f mUp;         // up vector
    float mFocusDistance;
};

//----------------------------------------------------------------------------

OrbitCam::OrbitCam() : mCamera(std::make_unique<Camera>()) {}

OrbitCam::~OrbitCam() = default;

void 
OrbitCam::setRenderContext(const moonray::rndr::RenderContext &context)
{
    mRenderContext = &context;
}

Mat4f
OrbitCam::resetTransform(const Mat4f &xform, const bool makeDefault)
{
    MNRY_ASSERT(mCamera);

    mCamera->mPosition = asVec3(xform.vw);
    mCamera->mViewDir = normalize(asVec3(-xform.vz));
    mCamera->mUp = asVec3(xform.vy);
    mCamera->mFocusDistance = 1.0f;

    if (!mInitialTransformSet || makeDefault) {
        mInitialTransformSet = true;
        mInitialPosition = mCamera->mPosition;
        mInitialViewDir = mCamera->mViewDir;
        mInitialUp = mCamera->mUp;
        mInitialFocusDistance = mCamera->mFocusDistance;
        mInitialFocusDistanceNeedsUpdate = true;
    }

    // We must reset the focus every time we 
    // reset the camera transform.
    mFocusSet = false;

    return xform;
}

void
OrbitCam::pickFocusPoint()
{
    MNRY_ASSERT(mCamera);
    
    // Ensure the render context exists.
    // Key bindings can call this function before everything is fully ready.
    if (!mRenderContext) { return; }
    
    // Do this function only once every time we reset the transform
    // Note: We can't do picking during resetTransform() because picking uses
    // the pbr Scene, which might not be initialized at that time.
    if (mFocusSet) { return; }
    mFocusSet = true;

    const scene_rdl2::math::HalfOpenViewport vp = mRenderContext->getRezedRegionWindow();
    const int width = int(vp.width());
    const int height = int(vp.height());
    Vec3f focusPoint;
    if (pick(width / 2, height / 2, &focusPoint)) {
        // Only set the focus distance; re-aiming at the (slightly off-axis) hit would change the camera xform.
        const float focusDistance = dot(focusPoint - mCamera->mPosition, mCamera->mViewDir);
        if (focusDistance > 0.0f) {
            mCamera->mFocusDistance = focusDistance;
        }
    }
    if (mInitialFocusDistanceNeedsUpdate) {
        mInitialFocusDistance = mCamera->mFocusDistance;
        mInitialFocusDistanceNeedsUpdate = false;
    }
}

Mat4f
OrbitCam::update(const float dt)
{
    const float movement = mSpeed * dt;

    // Process keyboard input.
    if (mInputState & ORBIT_FORWARD) {
        mCamera->translate(0.0f, 0.0f, -movement);
    }
    if (mInputState & ORBIT_BACKWARD) {
        mCamera->translate(0.0f, 0.0f, movement);
    }
    if (mInputState & ORBIT_LEFT) {
        mCamera->translate(movement, 0.0f, 0.0f);
    }
    if (mInputState & ORBIT_RIGHT) {
        mCamera->translate(-movement, 0.0f, 0.0f);
    }
    if (mInputState & ORBIT_UP) {
        mCamera->translate(0.0f, movement, 0.0f);
    }
    if (mInputState & ORBIT_DOWN) {
        mCamera->translate(0.0f, -movement, 0.0f);
    }
    if (mInputState & ORBIT_SLOW_DOWN) {
        mSpeed += -mSpeed * dt;
    }
    if (mInputState & ORBIT_SPEED_UP) {
        mSpeed += mSpeed * dt;
    }

    return makeCameraMatrix(*mCamera);
}

bool
OrbitCam::processKeyPressEvent(GLFWwindow* window, const Action action)
{
    // Need to get the mouse position to recenter the camera.
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mMouseX = static_cast<int>(xpos);
    mMouseY = static_cast<int>(ypos);

    pickFocusPoint();

    // Check for pressed keys.
    switch (action) {
    case ACTION_CAM_FORWARD:        mInputState |= ORBIT_FORWARD;           return true;
    case ACTION_CAM_BACKWARD:       mInputState |= ORBIT_BACKWARD;          return true;
    case ACTION_CAM_LEFT:           mInputState |= ORBIT_LEFT;              return true;
    case ACTION_CAM_RIGHT:          mInputState |= ORBIT_RIGHT;             return true;
    case ACTION_CAM_UP:             mInputState |= ORBIT_UP;                return true;
    case ACTION_CAM_DOWN:           mInputState |= ORBIT_DOWN;              return true;
    case ACTION_CAM_SLOW_DOWN:      mInputState |= ORBIT_SLOW_DOWN;         return true;
    case ACTION_CAM_SPEED_UP:       mInputState |= ORBIT_SPEED_UP;          return true;
    case ACTION_CAM_RECENTER:       recenterCamera();                       return true;
    case ACTION_CAM_RESET:          resetCamera();                          return true;
    case ACTION_CAM_FRAME_SCENE:    frameScene();                           return true;
    case ACTION_CAM_SET_UP_VECTOR:  mCamera->mUp = Vec3f(0.0f, 1.0f, 0.0f); return true;
    case ACTION_CAM_PRINT_MATRICES: printCameraMatrices();                  return true;
    case ACTION_CAM_ROTATE:         mMouseMode = ROTATE;                    return true;
    case ACTION_CAM_TRACK:          mMouseMode = TRACK;                     return true;
    case ACTION_CAM_DOLLY:          mMouseMode = DOLLY;                     return true;
    case ACTION_CAM_ROLL:           mMouseMode = ROLL;                      return true;
    }
    return false;
}

bool
OrbitCam::processKeyReleaseEvent(GLFWwindow* window, const Action action)
{
    mMouseMode = NONE;

    // Check for released keys.
    switch (action) {
    case ACTION_CAM_FORWARD:        mInputState &= ~ORBIT_FORWARD;    return true;
    case ACTION_CAM_BACKWARD:       mInputState &= ~ORBIT_BACKWARD;   return true;
    case ACTION_CAM_LEFT:           mInputState &= ~ORBIT_LEFT;       return true;
    case ACTION_CAM_RIGHT:          mInputState &= ~ORBIT_RIGHT;      return true;
    case ACTION_CAM_UP:             mInputState &= ~ORBIT_UP;         return true;
    case ACTION_CAM_DOWN:           mInputState &= ~ORBIT_DOWN;       return true;
    case ACTION_CAM_SLOW_DOWN:      mInputState &= ~ORBIT_SLOW_DOWN;  return true;
    case ACTION_CAM_SPEED_UP:       mInputState &= ~ORBIT_SPEED_UP;   return true;
    }
    return false;
}

bool
OrbitCam::processMouseMoveEvent(const double xpos, const double ypos)
{
    if (mMouseX == -1 || mMouseY == -1) {
        return false;
    }

    const int x = static_cast<int>(xpos);
    const int y = static_cast<int>(ypos);
    const float dClickX = static_cast<float>(x - mMouseX);
    const float dClickY = static_cast<float>(y - mMouseY);
    mMouseX = x;
    mMouseY = y;

    switch (mMouseMode) {
    case ROTATE:        mCamera->rotateOrbit(dClickX, dClickY);     return true;
    case TRACK:         mCamera->translate(dClickX, dClickY, 0.0f); return true;
    case DOLLY:         mCamera->dolly(dClickX + dClickY);          return true;
    case ROLL:          mCamera->roll(dClickX);                     return true;
    }

    return false;
}

void
OrbitCam::clearMovementState()
{
    mInputState = 0;
    mMouseMode = NONE;
    mMouseX = -1;
    mMouseY = -1;
}

void
OrbitCam::recenterCamera()
{
    if (mMouseX == -1 || mMouseY == -1) {
        return;
    }

    Vec3f newFocus;
    if (pick(mMouseX, mMouseY, &newFocus)) {
        Vec3f delta = newFocus - (mCamera->mPosition + mCamera->mViewDir * mCamera->mFocusDistance);
        mCamera->mPosition += delta;
        mCamera->mFocusDistance = length(newFocus - mCamera->mPosition);
    }

    // reset mouse positions so repeatedly pressing F does not result in
    // repeated recentering.
    mMouseX = mMouseY = -1;
}

void
OrbitCam::resetCamera()
{
    if (mInitialTransformSet) {
        clearMovementState();
        mCamera->mPosition = mInitialPosition;
        mCamera->mViewDir = mInitialViewDir;
        mCamera->mUp = mInitialUp;
        mCamera->mFocusDistance = mInitialFocusDistance;
    }
}

void
OrbitCam::frameScene()
{
    MNRY_ASSERT(mRenderContext);
    if (!mRenderContext) {
        return;
    }
    const scene_rdl2::math::BBox3f sceneBounds = mRenderContext->getSceneBoundsWorld();

    const float sceneSize = sceneBounds.size().length();
    if (sceneSize < scene_rdl2::math::sEpsilon) {
        return; // Avoid framing if the scene is too small or degenerate
    }

    mCamera->mViewDir = NavigationCam::FRAME_CAM_VIEW; // already normalized
    mCamera->mUp = scene_rdl2::math::Vec3f(0.0f, 1.0f, 0.0f);

    // Then, position the camera so that we can see the full scene.
    mCamera->mFocusDistance = sceneSize;
    const scene_rdl2::math::Vec3f sceneCenter = center(sceneBounds);

    mCamera->mPosition = sceneCenter - mCamera->mViewDir * mCamera->mFocusDistance;
}

bool
OrbitCam::pick(const int x, const int y, Vec3f* hitPoint) const
{
    MNRY_ASSERT(mRenderContext);

    // must use offset between center point of aperture window and center point
    // of region window so that the region window is centered on the pick point.
    const scene_rdl2::math::HalfOpenViewport avp = mRenderContext->getRezedApertureWindow();
    const scene_rdl2::math::HalfOpenViewport rvp = mRenderContext->getRezedRegionWindow();
    const int offsetX = (avp.max().x + avp.min().x) / 2 - (rvp.max().x + rvp.min().x) / 2;
    const int offsetY = (avp.max().y + avp.min().y) / 2 - (rvp.max().y + rvp.min().y) / 2;

    return mRenderContext->handlePickLocation(x + offsetX, y - offsetY, hitPoint);
}

Mat4f
OrbitCam::makeCameraMatrix(const Camera& camera) const
{
    const Xform3f c2w = camera.camera2world();
    return Mat4f( Vec4f(c2w.l.vx.x, c2w.l.vx.y, c2w.l.vx.z, 0.0f),
                  Vec4f(c2w.l.vy.x, c2w.l.vy.y, c2w.l.vy.z, 0.0f),
                  Vec4f(c2w.l.vz.x, c2w.l.vz.y, c2w.l.vz.z, 0.0f),
                  Vec4f(c2w.p.x,    c2w.p.y,    c2w.p.z,    1.0f) );
}

void
OrbitCam::printCameraMatrices() const
{
    const Mat4f fullMat = makeCameraMatrix(*mCamera);
    printMatrix("Full matrix containing rotation and position.", fullMat);
}

//----------------------------------------------------------------------------

} // namespace moonray_gui_v2

