# 🚀 Project Roadmap: Startup Phase

This document outlines the high-level roadmap for the initial phase of the project, focusing on core infrastructure and foundational systems.

---

## 🛠️ Phase 1: Core Infrastructure

| Step | Milestone | Key Focus Areas | Status |
| :--- | :--- | :--- | :--- |
| **1** | **Initial Build and Project Structure** | Establish build system, folder structure, dependency management, and basic CI/CD (if applicable). | **To Do** |
| **2** | **Core Application Structure** | Define the main application loop, module system, memory allocators, and basic object/component patterns. | **To Do** |

---

## ⚙️ Phase 2: Performance and Concurrency Systems

| Step | Milestone | Key Focus Areas | Status |
| :--- | :--- | :--- | :--- |
| **3** | **Task System Implementation** | Develop a high-performance, multithreaded task scheduler supporting **scheduling**, **task stealing**, and lightweight **fibers** for cooperative concurrency. | **To Do** |
| **4** | **Test Framework** | Implement a robust unit and integration testing framework to ensure code quality and stability. | **To Do** |
| **5** | **Test the Task System** | Rigorous testing and performance benchmarking of the new task system under various load conditions. | **To Do** |

---

## ⏳ Phase 3: Engine Fundamentals and Utilities

| Step | Milestone | Key Focus Areas | Status |
| :--- | :--- | :--- | :--- |
| **6** | **Engine Expansion (Clock & Frames)** | Create the concept of **"frames"** (for render/update cycles) and implement the core **engine simulation clock** for time management and determinism. | **To Do** |
| **7** | **High Performance Logging System** | Implement a fast, asynchronous, and lock-free logging system designed for minimal overhead in performance-critical environments. | **To Do** |

---

## ⌨️ Phase 4: User Interaction Foundation

| Step | Milestone | Key Focus Areas | Status |
| :--- | :--- | :--- | :--- |
| **8** | **Window Creation** | Setup the necessary platform abstraction to create and manage the main application window (e.g., using GLFW, SDL, or native APIs). | **To Do** |
| **9** | **Input System** | Implement a low-level system for polling and handling raw keyboard, mouse, and controller events. | **To Do** |
| **10** | **Input Action System** | Develop a higher-level, customizable system to map raw input events to game/application actions (e.g., "Jump," "Move Forward"). | **To Do** |

---

## 🖼️ Phase 5: Rendering Deep Dive (Major Task)

| Step | Milestone | Key Focus Areas | Status |
| :--- | :--- | :--- | :--- |
| **11** | **Renderer Deep Dive (Vulkan)** | **Enormous Task.** Focus on setting up the Vulkan API. Key areas include: | **To Do** |
| | | - Instance and device creation. | |
| | | - **Dynamic Pipelines** for flexible rendering. | |
| | | - Robust **GPU Resource Management** (buffers, textures, descriptors). | |
| | | - Synchronization primitives and command buffer submission. | |

## 📂 Phase 6: Asset Management and Streaming

| Step | Milestone | Key Focus Areas | Status |
| :--- | :--- | :--- | :--- |
| **12** | **Custom Asset Streaming Format** | Define and implement a **chunk-based** format for reading and streaming large assets efficiently off disk, optimizing for sequential and random access. | **To Do** |
| **13** | **Asset System and Asset Pipeline** | Develop the runtime **Asset System** (loading, unloading, referencing) and the **Asset Pipeline** (tools for converting source assets into the custom streaming format). | **To Do** |
| **14** | **Asset Lifetimes** | Implement robust reference counting and garbage collection/unloading policies to manage the memory lifetime of streamed assets, preventing leaks and ensuring data is available when needed. | **To Do** |