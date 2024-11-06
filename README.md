## 羲和 Xi He

羲和是按照《Mastering Graphics Programming with Vulkan》一书（作者：Marco Castorina 和 Gabriel Assone）的思路开发，整合了最先进技术的现代渲染引擎。

Xi He is developed based on the ideas presented in the book *Mastering Graphics Programming with Vulkan* by Marco Castorina and Gabriel Assone. It is a modern rendering engine that integrates state-of-the-art techniques.

## 基础框架 (Foundation Framework)

羲和渲染引擎的基础框架来自于 [KhronosGroup/Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples) 的 framework，只保留了 vulkan-hpp 的部分，基本不使用原始 C 接口，且相关框架都是从头按需构建的。

The foundation framework of the Xihe Rendering Engine is based on the [KhronosGroup/Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples) framework. Only the vulkan-hpp part is retained, avoiding the raw C interface as much as possible. Related frameworks are built from scratch as needed.

## 已实现功能 (Implemented Features)

- **自动化管线** / Automating pipeline
- **管线缓存** / Pipeline cache
- **无绑定渲染** / Bindless rendering
- **多线程记录命令** (Secondary command buffer) / Multi-threaded command recording (Secondary command buffer)
- **帧图** / Frame Graph
- **异步计算** / Async compute
- **级联阴影映射** / Cascaded shadow mapping
- **Mesh shader 几何管线** (生成 meshlet) / Mesh shader geometry pipeline (meshlet generation)

其中，自动化管线和管线缓存功能来自于 [KhronosGroup/Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples) 的 framework。

The automation pipeline and pipeline cache are based on the framework provided by [KhronosGroup/Vulkan-Samples](https://github.com/KhronosGroup/Vulkan-Samples).

## 待实现功能 (Features to be Implemented)

- **GPU 剔除** / GPU culling
- **聚簇延迟渲染** / Clustered Deferred Rendering
- **Mesh shader 阴影** / Mesh shader shadows
- **可变速率着色** / Variable Rate Shading (VRS)
- **体积雾** / Volumetric fog
- **时间抗锯齿** / Temporal Anti-Aliasing (TAA)
- **全局光照** / Global Illumination (GI)

##
![meshlet](./assets/images/meshlet.png)