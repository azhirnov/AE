# My R&D Engine
[![GitHub license](https://img.shields.io/github/license/azhirnov/AE.svg)](https://github.com/azhirnov/AE/blob/master/LICENSE)
[![Build Status](https://img.shields.io/travis/azhirnov/AE/master.svg?logo=travis)](https://travis-ci.com/azhirnov/AE)
[![GitHub issues](https://img.shields.io/github/issues/azhirnov/AE.svg)](https://github.com/azhirnov/AE/issues)
[![Donate](https://img.shields.io/badge/donate-PayPal-orange.svg?logo=paypal)](https://paypal.me/azhirnovgithub)
</br>


## Features
 * ECS with archetypes (like in Unity Engine)
 * Programmable rendering pipeline (Render graph)
 * Universal task system for ECS, RenderGraph, Network and other.

## Suported Platforms
* Windows (with MSVC 2017)
* Android 7.0+


## Building
Generate project with CMake and build.<br/>
Required C++17 standard support and CMake 3.10 or later.


## References
ECS:<br/>
* [Unity ECS](https://docs.unity3d.com/Packages/com.unity.entities@0.5/manual/index.html)
* [ECSTest project](https://github.com/Industrialice/ECSTest)
* [ECS FAQ](https://github.com/SanderMertens/ecs-faq#what-are-examples-of-ecs-implementations)
Renderer:<br/>
* [Destiny renderer](http://advances.realtimerendering.com/destiny/gdc_2015/Tatarchuk_GDC_2015__Destiny_Renderer_web.pdf)
* [FrameGraph by Ubisoft](https://developer.download.nvidia.com/assets/gameworks/downloads/regular/GDC17/DX12CaseStudies_GDC2017_FINAL.pdf)
* [RenderGraph by EA](https://www.khronos.org/assets/uploads/developers/library/2019-reboot-develop-blue/SEED-EA_Rapid-Innovation-Using-Modern-Graphics_Apr19.pdf)
