#pragma once
#include <GLFW/glfw3.h>

void center_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void updateMovement(GLFWwindow* window, float dt);
void updateMapMovement(GLFWwindow* window, float dt);



