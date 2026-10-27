#pragma once
// Reflection cube maps (docs/re/rendering-reflections.md): the material setup 0x129560 sets the reflection flag 0x37796d for texture
// bytes 5, 6 and 7, the texture binding 0x12df94 then binds the slots in reverse and makes a cube texture of unit 0 a cube map, and the
// vertex programs write the reflection vector (subroutine 0x39 of the shader blob, transposed model matrix as `TexTrans`) to texture
// coordinate 0.

namespace chase::runtime {

enum class UnitSampling { None, Texture, Cube, Disabled };

// 0x12df94: a cube texture is bound only while the reflection flag is set and then as the cube map of unit 0; outside reflection
// materials its unit stays switched off.
UnitSampling unitSampling(int unit, bool reflection, bool hasTexture, bool cubeTexture);

// Texture coordinate set (0 = uv0, 1 = uv1) of texture unit 1 or 2. With the reflection flag the vertex program writes uv0 to texture
// coordinate 1 and uv1 to texture coordinate 2 (subroutine 0x39), so unit 1 samples uv0. Without the flag the assumed mapping of
// stand-in RENDER-S4 stays.
int unitUvSet(int unit, bool reflection, bool hasSecondUv);

enum class CubeFace { PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ };

// The six faces of a cube texture follow each other in the file; 0x12df94 writes face k to the PICA address register 0x85 + k. Which
// direction each register stands for is the PICA convention as Citra and Azahar emulate it (stand-in RENDER-S69).
CubeFace cubeFace(int storedIndex);

}
