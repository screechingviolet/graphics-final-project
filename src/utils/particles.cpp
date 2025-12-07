#include "realtime.h"

// Finds a Particle in ParticlesContainer which isn't used yet.
// (i.e. life < 0);
int Realtime::FindUnusedParticle() {

    for (int i = m_lastUsedParticle; i < m_maxNumParticles; i++){
        if (m_particles[i].life < 0){
            m_lastUsedParticle = i;
            return i;
        }
    }

    for(int i = 0; i < m_lastUsedParticle; i++){
        if (m_particles[i].life < 0){
            m_lastUsedParticle = i;
            return i;
        }
    }

    return 0; // All particles are taken, override the first one
}

void Realtime::particleUpdate() {
    m_numParticles = 0;

    for (int i = 0; i < m_maxNumParticles; i++) {
        Particle& p = m_particles[i]; // shortcut

        if (p.life > 0.0f) {
            // Decrease life
            p.life -= m_dt;

            if (p.life > 0.0f) {
                // Simulate simple physics : gravity only, no collisions
                //p.velocity += glm::vec3(0.0f, -9.81f, 0.0f) * (float)m_dt * 0.1f;
                //commented out for debugging
                p.pos += p.velocity * (float)m_dt * m_particleVelocityGlobal;
                p.cameraDistance = glm::length(p.pos - glm::vec3(0, 0, 0));
                //ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

                // Fill the GPU buffer
                for (int j = 0; j < 3; j++) {
                    m_particulePositionSizeData[4 * m_numParticles + j] = p.pos[j];
                }

                m_particulePositionSizeData[4 * m_numParticles + 3] = p.size;

                for (int j = 0; j < 4; j++) {
                    m_particuleColorData[4 * m_numParticles + j] = p.color[j];
                }
            } else {
                // Particles that just died will be put at the end of the buffer in SortParticles();
                p.cameraDistance = -1.0f;
            }

            m_numParticles++;
        } else {
            m_particles[i] = Particle(glm::vec3(0, 0, 0));
            m_numParticles++;
        }
    }
}
