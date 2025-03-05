#include <vector>
#include <cmath>
#include <cstdlib>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>

float separation_radius = 10.0;
float alignment_radius = 10.0;
float cohesion_radius = 10.0;
float w_sep = 0.005;
float w_al = 0.000001;
float w_coh = 0.000001;
float al_efect = 20.0;
float coh_efect = 2.0;      //podobne do w_coh, ale troche inne
float border = 10.0;        //granica swiata boidow
float velocity = 2.0;       //predkosc boidow

int leader = 0;             //lider boidow


std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

struct Boid {
    float x, y, z, vx, vy, vz;
    Boid(float a, float b, float c, float d, float e, float f) {
        x = a;
        y = b;
        z = c;
        vx = d;
        vy = e;
        vz = f;
        std::cout << "Boid nr" << ": "<< "(" << x << ", " << y << ", " << z << ")" << "\n";
    }
};

std::vector<Boid> boids;

void initBoids(int count, float width, float height, float depth) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> coords(-3, 3);               //generacja losowej pozycji
    boids.clear();
    for (int i = 0; i < count; i++) {
        boids.emplace_back(coords(gen), coords(gen), coords(gen), 0.005, 0.005, 0.005);
    }
}

glm::vec3 calculate_separation(Boid boid) {
    int count = 0;
    glm::vec3 F_sep = glm::vec3(0, 0, 0);
    for (auto other_boid : boids){
        if (boid.x != other_boid.x || boid.y != other_boid.y || boid.z != other_boid.z) {
            glm::vec3 d = glm::vec3(boid.x - other_boid.x, boid.y - other_boid.y, boid.z - other_boid.z);
            float distance = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            if (distance < separation_radius && distance != 0) {
                glm::vec3 d_norm = glm::vec3(d.x / distance, d.y / distance, d.z / distance);
                glm::vec3 F = glm::vec3(d_norm.x / distance, d_norm.y / distance, d_norm.z / distance);
                F_sep = glm::vec3(F_sep.x + F.x, F_sep.y + F.y, F_sep.z + F.z);
                count++;
            }
        }
    }
    float F_len = std::sqrt(F_sep.x * F_sep.x + F_sep.y * F_sep.y + F_sep.z * F_sep.z);
    if (F_len != 0) {
        F_sep = w_sep * F_sep / F_len;
    }

    if (count > 0) {
        F_sep = F_sep / count;
    }
    return F_sep;
}

glm::vec3 calculate_alignment(Boid boid) {
    glm::vec3 F_al = glm::vec3(0, 0, 0);
    int neighbours_count = 0;
    float vx = 0, vy = 0, vz = 0;
    float avg_vx, avg_vy, avg_vz;
    for (auto other_boid : boids) {
        if (boid.x != other_boid.x || boid.y != other_boid.y || boid.z != other_boid.z) {
            glm::vec3 d = glm::vec3(boid.x - other_boid.x, boid.y - other_boid.y, boid.z - other_boid.z);
            float distance = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            if (distance < alignment_radius) {
                vx += other_boid.vx;
                vy += other_boid.vy;
                vz += other_boid.vz;
                neighbours_count += 1;
            }
        }
    }
    if (neighbours_count > 0) {
        avg_vx = vx / neighbours_count;
        avg_vy = vy / neighbours_count;
        avg_vz = vz / neighbours_count;

        float x = boid.vx + (avg_vx - boid.vx) * al_efect;
        float y = boid.vy + (avg_vy - boid.vy) * al_efect;
        float z = boid.vz + (avg_vz - boid.vz) * al_efect;

        F_al = glm::vec3(x, y, z);
    }
    return F_al;
}

glm::vec3 calculate_cohesion(Boid boid) {
    glm::vec3 F_coh = glm::vec3(0, 0, 0);
    int neighbours_count = 0;
    float sum_x = 0, sum_y = 0, sum_z = 0;
    float avg_x, avg_y, avg_z;
    for (auto other_boid : boids) {
        if (boid.x != other_boid.x || boid.y != other_boid.y || boid.z != other_boid.z) {
            glm::vec3 d = glm::vec3(boid.x - other_boid.x, boid.y - other_boid.y, boid.z - other_boid.z);
            float distance = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
            if (distance < cohesion_radius) {
                sum_x += other_boid.x;
                sum_y += other_boid.y;
                sum_z += other_boid.z;
                neighbours_count += 1;
            }
        }
    }
    if (neighbours_count > 0) {
        avg_x = sum_x / neighbours_count;
        avg_y = sum_y / neighbours_count;
        avg_z = sum_z / neighbours_count;

        /*
        float dst = std::sqrt(avg_x * avg_x + avg_y * avg_y + avg_z * avg_z);
        if (dst > 0) {
            avg_x = avg_x / dst;
            avg_y = avg_y / dst;
            avg_z = avg_z / dst;
        }
        */


        glm::vec3 F = glm::vec3(boids[leader].x - boid.x, boids[leader].y - boid.y, boids[leader].z - boid.z);
        F_coh = coh_efect * F;
    }
    return F_coh;
}

void update_boids() {
    int num = 0;
    for (auto &boid : boids) {
        glm::vec3 F_sep = calculate_separation(boid);
        glm::vec3 F_al = calculate_alignment(boid);
        glm::vec3 F_coh = calculate_cohesion(boid);
        glm::vec3 F = glm::vec3(0, 0, 0);

        if (num != leader) {
            F = F + w_sep * F_sep + w_al * F_al + w_coh * F_coh;
        }
        if (num == leader) {
            F = F + w_al * F_al;
        }
        
        boid.vx += F.x;
        boid.vy += F.y;
        boid.vz += F.z;


        boid.x += boid.vx * velocity;
        boid.y += boid.vy * velocity;
        boid.z += boid.vz * velocity;

        if (boid.x > border) {
            boid.x = border - 0.001;
            boid.vx = boid.vx * -1;
        }

        if (boid.x < border * -1) {
            boid.x = -1 * border + 0.001;
            boid.vx = boid.vx * -1;
        }

        if (boid.y > border) {
            boid.y = border - 0.001;
            boid.vy = boid.vy * -1;
        }

        if (boid.y < border * -1) {
            boid.y = -1 * border + 0.001;
            boid.vy = boid.vy * -1;
        }

        if (boid.z > border) {
            boid.z = border - 0.001;
            boid.vz = boid.vz * -1;
        }

        if (boid.z < border * -1) {
            boid.z = -1 * border + 0.001;
            boid.vz = boid.vz * -1;
        }
        num++;
    }
}