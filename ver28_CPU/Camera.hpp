#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float FOV = 45.0f;           // field of view

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera {
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;    // gaze
    glm::vec3 Up;       // top
    glm::vec3 Right;    // gaze cross product top
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float fov;

    int width;
    int height;

    float lastX;
    float lastY;
    bool firstMouse = true;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.f, 0.f, 0.f), int WIDTH = 800, int HEIGHT = 600, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : 
        Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), fov(FOV) {

        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        width = WIDTH;
        height = HEIGHT;
        lastX = width / 2.0f;
        lastY = height / 2.0f;

        updateCamera();
    }

    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), fov(FOV)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCamera();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }


    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xposIn, float yposIn, GLboolean constrainPitch = true)
    {
        if (firstMouse) // initially true, it's like a state, only call these when being first called
        {
            lastX = xposIn;
            lastY = yposIn;
            firstMouse = false;
        }

        float xoffset = xposIn - lastX;
        float yoffset = lastY - yposIn; // reversed since y-coordinates go from bottom to top
        lastX = xposIn;
        lastY = yposIn;

        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCamera();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        fov -= (float)yoffset;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 45.0f)
            fov = 45.0f;
    }


public:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCamera()
    {
        // calculate the new Front vector
        //glm::vec3 front;
        //front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        //front.y = sin(glm::radians(Pitch));
        //front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        //this->Front = glm::normalize(front);

        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(this->Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, this->Front));
    }
};