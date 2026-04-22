    /*
     * ======================================================================================
     * INTEGER-ONLY SOFTWARE 3D RENDERER (Retro-Inspired)
     * ======================================================================================
     * * DESIGN PHILOSOPHY:
     * This program demonstrates how 3D graphics were handled in the 80s/90s without
     * specialized hardware (GPUs). I treat the screen as a linear array of memory
     * (a "framebuffer") and perform all calculations using fixed-point integer math.
     * * WHY INTEGER MATH?
     * 1. Performance: Older CPUs lacked Floating-Point Units (FPUs).
     * 2. Determinism: You get identical results across all hardware.
     * 3. Cache-friendliness: It is easier for the CPU to predict access patterns.
     * * CORE PIPELINE:
     * 1. DATA: Define the 3D cube model (vertices and face indices).
     * 2. TRANSFORM: Perform matrix rotations and project 3D space into 2D screenspace.
     * 3. RASTERIZE: Fill the triangles while testing Z-depth to ensure proper occlusion.
     * 4. PRESENT: Push the finished buffer into an SDL texture for display.
     */

    #include <SDL2/SDL.h>
    #include <math.h>
    #include <stdlib.h>

    // Screen dimensions
    #define WIDTH 800
    #define HEIGHT 600

    // Fixed-point settings: I shift integers left by 16 bits to store decimal-like precision.
    // This makes 1.0 = (1 << 16) = 65536.
    #define FP_SHIFT 16
    #define FP_UNIT (1 << FP_SHIFT)

    typedef struct { int x, y, z; } Vec3;

    // Look-up Table (LUT) for sine values.
    // Calculating sin() at runtime is extremely slow; pre-calculating it into an array
    // at startup is a standard demo-scene optimization.
    int sin_lut[1024];

    void init_lut() {
        for (int i = 0; i < 1024; i++)
            sin_lut[i] = (int)(sinf(i * 2.0f * M_PI / 1024.0f) * FP_UNIT);
    }

    // Map any integer angle to our 1024-step sine table using bitwise wrapping.
    int get_sin(int a) { return sin_lut[a & 1023]; }
    // Cosine is just a phase-shifted sine (90 degrees or 256 LUT steps).
    int get_cos(int a) { return sin_lut[(a + 256) & 1023]; }

    // Fixed-point multiplication:
    // (A * B) produces a 64-bit integer. Because both numbers are scaled by FP_SHIFT,
    // the result is scaled by (FP_SHIFT * 2). I shift right by FP_SHIFT to fix the scale.
    int fp_mul(long long a, long long b) { return (int)((a * b) >> FP_SHIFT); }

    /*
     * TRIANGLE RASTERIZER (Barycentric Coordinate Method)
     * ---------------------------------------------------
     * This function fills the pixels inside a 2D triangle.
     */
    void draw_triangle(Uint32* pixels, int* zbuffer, Vec3 v0, Vec3 v1, Vec3 v2, Uint32 color) {
        // 1. BACK-FACE CULLING
        // Calculate the signed area via Cross Product. If area is positive,
        // the face points away from the camera and should not be drawn.
        long long area = (long long)(v1.x - v0.x) * (v2.y - v0.y) - (long long)(v1.y - v0.y) * (v2.x - v0.x);
        if (area >= 0) return;

        // 2. BOUNDING BOX
        // Only check pixels inside the smallest rectangle that contains the triangle.
        // This optimization skips thousands of unnecessary point-in-triangle tests.
        int min_x = fmin(v0.x, fmin(v1.x, v2.x)); int max_x = fmax(v0.x, fmax(v1.x, v2.x));
        int min_y = fmin(v0.y, fmin(v1.y, v2.y)); int max_y = fmax(v0.y, fmax(v1.y, v2.y));

        // 3. SCAN-CONVERSION
        for (int y = (min_y < 0 ? 0 : min_y); y <= (max_y >= HEIGHT ? HEIGHT - 1 : max_y); y++) {
            for (int x = (min_x < 0 ? 0 : min_x); x <= (max_x >= WIDTH ? WIDTH - 1 : max_x); x++) {

                // Edge functions (W):
                // These determine if the pixel (x,y) is on the correct side of each triangle edge.
                long long w0 = (long long)(v2.x - v1.x) * (y - v1.y) - (long long)(v2.y - v1.y) * (x - v1.x);
                long long w1 = (long long)(v0.x - v2.x) * (y - v2.y) - (long long)(v0.y - v2.y) * (x - v2.x);
                long long w2 = (long long)(v1.x - v0.x) * (y - v0.y) - (long long)(v1.y - v0.y) * (x - v0.x);

                // If all weights are negative, the point is inside the triangle.
                if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                    // Z-Depth Interpolation:
                    // Calculate the pixel's depth based on its barycentric weights (W)
                    // relative to the vertex depths (v0.z, v1.z, v2.z).
                    int z = (int)((w0 * v0.z + w1 * v1.z + w2 * v2.z) / area);
                    int idx = y * WIDTH + x;

                    // Z-Buffer check: Only write to the pixel buffer if this point is closer.
                    if (z < zbuffer[idx]) {
                        zbuffer[idx] = z;
                        pixels[idx] = color;
                    }
                }
            }
        }
    }

    int main(int argc, char* argv[]) {
        SDL_Init(SDL_INIT_VIDEO);
        init_lut();
        SDL_Window* win = SDL_CreateWindow("Software Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
        SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

        Uint32* pixels = malloc(WIDTH * HEIGHT * 4);
        int* zbuffer = malloc(WIDTH * HEIGHT * sizeof(int));

        // Cube Model Definition: 8 vertices at unit distance from center.
        Vec3 model[8] = {{-FP_UNIT,-FP_UNIT, FP_UNIT}, { FP_UNIT,-FP_UNIT, FP_UNIT}, { FP_UNIT, FP_UNIT, FP_UNIT}, {-FP_UNIT, FP_UNIT, FP_UNIT},
                         {-FP_UNIT,-FP_UNIT,-FP_UNIT}, { FP_UNIT,-FP_UNIT,-FP_UNIT}, { FP_UNIT, FP_UNIT,-FP_UNIT}, {-FP_UNIT, FP_UNIT,-FP_UNIT}};
        // Faces: Index pointers to the model vertices.
        int tris[12][3] = {{0,1,2}, {0,2,3}, {1,5,6}, {1,6,2}, {5,4,7}, {5,7,6}, {4,0,3}, {4,3,7}, {3,2,6}, {3,6,7}, {4,5,1}, {4,1,0}};
        Uint32 colors[6] = {0xFFFF4444, 0xFF44FF44, 0xFF4444FF, 0xFFFFCC44, 0xFFCC44FF, 0xFF44CCCC};

        int angle = 0;
        while (1) {
            SDL_Event e; if (SDL_PollEvent(&e) && e.type == SDL_QUIT) break;

            // 1. CLEAR FRAME
            // Reset pixel buffer (background black) and Z-buffer (infinite distance).
            for (int i = 0; i < WIDTH * HEIGHT; i++) { pixels[i] = 0xFF000000; zbuffer[i] = 0x7FFFFFFF; }

            angle += 4; // Incrementing angle creates the rotation motion.
            int s = get_sin(angle), c = get_cos(angle), s2 = get_sin(angle / 2), c2 = get_cos(angle / 2);

            // 2. RENDER LOOP
            for (int i = 0; i < 12; i++) {
                Vec3 p[3]; // Transformed 2D points
                for (int j = 0; j < 3; j++) {
                    Vec3 v = model[tris[i][j]];

                    // ROTATION: Linear algebra transformation matrices (using LUT sin/cos).
                    int tx = fp_mul(v.x, c) - fp_mul(v.z, s);
                    int tz = fp_mul(v.x, s) + fp_mul(v.z, c);
                    int ty = fp_mul(v.y, c2) - fp_mul(tz, s2);
                    tz = fp_mul(v.y, s2) + fp_mul(tz, c2);

                    // PERSPECTIVE PROJECTION:
                    // Offset the depth to keep the cube in view, then divide by (depth >> 8).
                    // Dividing by depth makes objects further away look smaller.
                    int depth = tz + (FP_UNIT * 6);
                    p[j].x = ((long long)tx * 450) / (depth >> 8) / 256 + (WIDTH / 2);
                    p[j].y = ((long long)ty * 450) / (depth >> 8) / 256 + (HEIGHT / 2);
                    p[j].z = depth;
                }
                // Draw the face using our custom software rasterizer.
                draw_triangle(pixels, zbuffer, p[0], p[1], p[2], colors[i/2]);
            }

            // 3. PRESENT
            // Move the raw memory pixel array to the screen.
            SDL_UpdateTexture(tex, NULL, pixels, WIDTH * 4);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, NULL, NULL);
            SDL_RenderPresent(ren);
            SDL_Delay(16); // Target ~60 FPS
        }
        return 0;
    }
