#pragma once
#include <GLFW/glfw3.h>

void center_callback(GLFWwindow* window, int button, int action, int mods);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void updateMovement(GLFWwindow* window, float dt);
void updateMapMovement(GLFWwindow* window, float dt);

// New: mouse movement callback (used for mouse-look when cursor is captured)
void mouse_move_callback(GLFWwindow* window, double xpos, double ypos);

// New: scroll callback to move camera vertically
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);



