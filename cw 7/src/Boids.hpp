#include <vector>
#include <cmath>
#include <cstdlib>
#include <GLFW/glfw3.h>

struct Boid {
    float x, y, vx, vy;
    Boid(float x, float y) : x(x), y(y),
        vx((rand() % 20 - 10) / 10.0f),
        vy((rand() % 20 - 10) / 10.0f) {}
};

std::vector<Boid> boids;

void initBoids(int count, float width, float height) {
    boids.clear();
    for (int i = 0; i < count; i++) {
        boids.emplace_back(rand() % int(width), rand() % int(height));
    }
}

void updateBoids() {
    for (auto& b : boids) {
        float ax = 0, ay = 0;

        for (const auto& other : boids) {
            if (&b == &other) continue;
            float dx = other.x - b.x, dy = other.y - b.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 50 && dist > 0) {  // If close enough, adjust velocity
                ax += dx * 0.01f;
                ay += dy * 0.01f;
            }
        }
        b.vx += ax;
        b.vy += ay;
        b.x += b.vx;
        b.y += b.vy;
        // Wrap around screen (assumes screen size of 800x600)
        if (b.x < 0) b.x = 800;
        if (b.x > 800) b.x = 0;
        if (b.y < 0) b.y = 600;
        if (b.y > 600) b.y = 0;
    }
}

void drawBoid(const Boid& b) {
    // Draw a simple triangle for each boid (legacy OpenGL)
    glBegin(GL_TRIANGLES);
    glVertex2f(b.x, b.y);
    glVertex2f(b.x - 5, b.y - 10);
    glVertex2f(b.x + 5, b.y - 10);
    glEnd();
}
