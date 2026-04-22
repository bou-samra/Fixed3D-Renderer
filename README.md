# Retro-Fixed3D: Integer-Only Software Renderer

A "back-to-basics" 3D engine written in C that eschews modern GPUs and floating-point math in favor of retro-inspired fixed-point arithmetic and manual rasterization.



## 🚀 Overview

This project is a educational deep-dive into how 3D graphics were handled in the 80s and 90s. Instead of relying on OpenGL, DirectX, or Vulkan, this renderer treats the screen as a raw linear array of memory (a framebuffer) and calculates every pixel manually using **Fixed-Point Integer Math**.

### Why Integer-Only?
* **Performance:** Historically, CPUs lacked Floating-Point Units (FPUs). Integer math was the only way to achieve real-time frame rates.
* **Determinism:** Eliminates rounding errors across different hardware architectures.
* **Educational Value:** Strips away the "magic" of modern APIs to show the raw geometry and trigonometry beneath the surface.

---

## 🛠️ Core Pipeline

The engine follows a classic software rendering pipeline:

1.  **Fixed-Point Math:** Uses bit-shifting (16.16 format) to simulate decimals without a performance hit.
2.  **LUT (Look-Up Tables):** Pre-calculates Sine and Cosine values at startup to avoid expensive runtime trigonometric calls.
3.  **Transformation:** Applies matrix rotation and perspective projection to convert 3D model coordinates into 2D screen space.
4.  **Back-Face Culling:** Uses cross-product area calculations to skip drawing faces pointing away from the camera.
5.  **Barycentric Rasterization:** Fills triangles by testing pixel containment and interpolating Z-depth for proper occlusion (Z-Buffering).

---

## 💻 Technical Highlights

### Fixed-Point Precision
Instead of `float`, we use `int` shifted by 16 bits.
$$1.0 = (1 \ll 16) = 65536$$

### The Rasterizer
The `draw_triangle` function implements a barycentric coordinate method to ensure sub-pixel accuracy and smooth depth testing.

```c
// Example of the edge function used for point-in-triangle testing
long long w0 = (long long)(v2.x - v1.x) * (y - v1.y) - (long long)(v2.y - v1.y) * (x - v1.x);
```

---

## 📥 Getting Started

### Prerequisites
* A C compiler (GCC/Clang)
* **SDL2** library (used only for creating the window and pushing the final pixel buffer to the screen)

### Building
```bash
# Example build command using GCC
gcc -o renderer main.c `sdl2-config --cflags --libs` -lm
```

### Controls
* **Close Window:** Exit the program.
* The cube rotates automatically to demonstrate the real-time transformation and depth buffering.

---

## 📜 License
This project is open-source and intended for educational purposes. Feel free to use it to learn more about the foundations of computer graphics.
