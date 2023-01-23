// Copyright (C) 2009-2022, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#if !defined(ANKI_SCENE_COMPONENT_SEPERATOR)
#	define ANKI_SCENE_COMPONENT_SEPERATOR
#endif

ANKI_DEFINE_SCENE_COMPONENT(Frustum, -1.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(GenericGpuComputeJob, -1.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Spatial, -1.0f)
ANKI_SCENE_COMPONENT_SEPERATOR

ANKI_DEFINE_SCENE_COMPONENT(Script, 0.0f)
ANKI_SCENE_COMPONENT_SEPERATOR

ANKI_DEFINE_SCENE_COMPONENT(Body, 10.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(PlayerController, 10.0f)
ANKI_SCENE_COMPONENT_SEPERATOR

ANKI_DEFINE_SCENE_COMPONENT(Joint, 10.0f)
ANKI_SCENE_COMPONENT_SEPERATOR

ANKI_DEFINE_SCENE_COMPONENT(Move, 30.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Skin, 30.0f)
ANKI_SCENE_COMPONENT_SEPERATOR

ANKI_DEFINE_SCENE_COMPONENT(Trigger, 40.0f)
ANKI_SCENE_COMPONENT_SEPERATOR

ANKI_DEFINE_SCENE_COMPONENT(Model, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(ParticleEmitter, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Decal, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Camera, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(FogDensity, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(GlobalIlluminationProbe, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(ReflectionProbe, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Skybox, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Ui, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(GpuParticleEmitter, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(LensFlare, 100.0f)
ANKI_SCENE_COMPONENT_SEPERATOR
ANKI_DEFINE_SCENE_COMPONENT(Light, 100.0f)

#undef ANKI_DEFINE_SCENE_COMPONENT
#undef ANKI_SCENE_COMPONENT_SEPERATOR
