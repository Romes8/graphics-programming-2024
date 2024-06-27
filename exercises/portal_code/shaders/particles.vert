#version 330 core

layout (location = 0) in vec2 ParticlePosition;
layout (location = 1) in float ParticleSize;
layout (location = 2) in float ParticleBirth;
layout (location = 3) in float ParticleDuration;
layout (location = 4) in vec4 ParticleColor;
layout (location = 5) in vec2 ParticleVelocity;
layout (location = 6) in vec2 ParticleOrientation;

out vec4 Color;
out vec4 EndPosition;

uniform float CurrentTime;
uniform float Gravity;

void main()
{
    Color = ParticleColor;
    float age = CurrentTime - ParticleBirth;

    if (age >= ParticleDuration) {
        // Make the particle transparent to not appear in the middle of the screen
        Color = vec4(ParticleColor.rgb, 0.0);
    } else {
        Color = ParticleColor;
    }

    // Calculate the particle's current position
    vec2 position = ParticlePosition;
    position += ParticleVelocity * age;
    position += 0.5f * vec2(0, 0) * age * age;

    // Calculate the direction based on orientation
    vec2 direction = normalize(ParticleOrientation);

    // Calculate the offset for the line segment
    // Adjust this value to make the lines shorter
    float lineLength = 0.1; // Example value to make lines short
    vec2 offset = vec2(0.0, 0.0); // Adjust this value as needed

    // Calculate the end points of the line segment
    vec2 startPos = position - offset;
    vec2 endPos = position + offset;

    // Output the start position
    gl_Position = vec4(startPos, 0.0, 1.0);

    // Pass the end position to the fragment shader
    EndPosition = vec4(endPos, 0.0, 1.0);
}