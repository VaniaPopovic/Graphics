//--------------------------------------------------------------------------------------
// File: GraphicsAssign1.fx
//
//	Shaders Graphics Assignment
//	Add further models using different shader techniques
//	See assignment specification for details
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

// Standard input geometry data, more complex techniques (e.g. normal mapping) may need more

struct LIGHTS_INPUT
{
    float3 Pos    : POSITION;
    float3 Normal : NORMAL;
	float2 UV     : TEXCOORD0;
};

// Data output from vertex shader to pixel shader for simple techniques. Again different techniques have different requirements
struct VS_BASIC_OUTPUT
{
    float4 ProjPos : SV_POSITION;
    float2 UV      : TEXCOORD0;
};

struct VS_NORMALMAP_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    float3 Tangent : TANGENT;
};

struct VS_LIGHTING_OUTPUT
{
    float4 ProjPos : SV_POSITION; // 2D "projected" position for vertex (required output for vertex shader)
    float3 WorldPos : POSITION;
    float3 WorldNormal : NORMAL;
    float2 UV : TEXCOORD0;
};
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 worldPos : POSITION;
    float2 TexCoord : TEXCOORD;
    float3 normal : NORMAL;
};
struct VS_NORMALMAP_OUTPUT
{
    float4 ProjPos : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 ModelNormal : NORMAL;
    float3 ModelTangent : TANGENT;
    float2 UV : TEXCOORD0;
};


struct LIGHTS_OUTPUT
{
    float4 DiffuseColour : DIFFUSE_COLOUR;
    float4 SpecularColour : SPECULAR_COLOUR;
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
// All these variables are created & manipulated in the C++ code and passed into the shader here

// The matrices (4x4 matrix of floats) for transforming from 3D model to 2D projection (used in vertex shader)
float4x4 WorldMatrix;
float4x4 ViewMatrix;
float4x4 ProjMatrix;
float4x4 ViewProjMatrix;

// A single colour for an entire model - used for light models and the intial basic shader
float3 ModelColour;
int NumberOfSpotLights;
float3 SpotLightPositions[3];
float3 SpotLightDirections[3];
float3 SpotLightColours[3];
float SpotLightAngles[3];
float SpotLightIntensities[3];

int NumberOfPointLights;
float3 PointLightPositions[3];
float4 PointLightColours[3];


// Information used for lighting (in the vertex or pixel shader)
float3 Light1Pos;
float3 Light2Pos;
float3 Light3Pos;
float3 Light1Colour;
float3 Light2Colour;
float3 Light3Color;
float3 AmbientColour;
float SpecularPower;
float3 CameraPos;

// Variable used to tint each light model to show the colour that it emits
float3 TintColour;

// Diffuse map (sometimes also containing specular map in alpha channel). Loaded from a bitmap in the C++ then sent over to the shader variable in the usual way
Texture2D DiffuseMap;
Texture2D CellMap;
//****| INFO | Normal map, now contains depth per pixel in the alpha channel ****//
Texture2D NormalMap;
//****| INFO | Also store a factor to strengthen/weaken the parallax effect. Cannot exaggerate it too much or will get distortion ****//
float ParallaxDepth;



float OutlineThickness;

// Sampler to use with the diffuse/normal maps. Specifies texture filtering and addressing mode to use when accessing texture pixels
SamplerState TrilinearWrap
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState PointSampleClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};


//--------------------------------------------------------------------------------------
// Vertex Shaders
//--------------------------------------------------------------------------------------

// Basic vertex shader to transform 3D model vertices to 2D and pass UVs to the pixel shader
//
VS_BASIC_OUTPUT BasicTransform(LIGHTS_INPUT vIn )
{
	VS_BASIC_OUTPUT vOut;
	
	// Use world matrix passed from C++ to transform the input model vertex position into world space
	float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
	float4 worldPos = mul( modelPos, WorldMatrix );
	float4 viewPos  = mul( worldPos, ViewMatrix );
	vOut.ProjPos    = mul( viewPos,  ProjMatrix );
	
	// Pass texture coordinates (UVs) on to the pixel shader
	vOut.UV = vIn.UV;

	return vOut;
}


float4 DiffuseTextured(VS_BASIC_OUTPUT vOut) : SV_Target
{
    return DiffuseMap.Sample(TrilinearWrap, vOut.UV); //Return the texture colour of this pixel
}


VS_NORMALMAP_OUTPUT NormalMapTransform(VS_NORMALMAP_INPUT vIn)
{
    VS_NORMALMAP_OUTPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
    float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
    float4 worldPos = mul(modelPos, WorldMatrix);
    vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
    float4 viewPos = mul(worldPos, ViewMatrix);
    vOut.ProjPos = mul(viewPos, ProjMatrix);

	// Just send the model's normal and tangent untransformed (in model space). The pixel shader will do the matrix work on normals
    vOut.ModelNormal = vIn.Normal;
    vOut.ModelTangent = vIn.Tangent;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
    vOut.UV = vIn.UV;

    return vOut;
}
VS_LIGHTING_OUTPUT VertexLightingTex(LIGHTS_INPUT vIn)
{
    VS_LIGHTING_OUTPUT vOut;

	// Use world matrix passed from C++ to transform the input model vertex position into world space
    float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
    float4 worldPos = mul(modelPos, WorldMatrix);
    vOut.WorldPos = worldPos.xyz;

	// Use camera matrices to further transform the vertex from world space into view space (camera's point of view) and finally into 2D "projection" space for rendering
    float4 viewPos = mul(worldPos, ViewMatrix);
    vOut.ProjPos = mul(viewPos, ProjMatrix);

	// Transform the vertex normal from model space into world space (almost same as first lines of code above)
    float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
    vOut.WorldNormal = mul(modelNormal, WorldMatrix).xyz;

	// Pass texture coordinates (UVs) on to the pixel shader, the vertex shader doesn't need them
    vOut.UV = vIn.UV;


    return vOut;
}
VS_BASIC_OUTPUT ExpandOutline(LIGHTS_INPUT vIn)
{
    VS_BASIC_OUTPUT vOut;

	// Transform model-space vertex position to world-space
    float4 modelPos = float4(vIn.Pos, 1.0f); // Promote to 1x4 so we can multiply by 4x4 matrix, put 1.0 in 4th element for a point (0.0 for a vector)
    float4 worldPos = mul(modelPos, WorldMatrix);

	// Next the usual transform from world space to camera space - but we don't go any further here - this will be used to help expand the outline
	// The result "viewPos" is the xyz position of the vertex as seen from the camera. The z component is the distance from the camera - that's useful...
    float4 viewPos = mul(worldPos, ViewMatrix);

	// Transform model normal to world space, using the normal to expand the geometry, not for lighting
    float4 modelNormal = float4(vIn.Normal, 0.0f); // Set 4th element to 0.0 this time as normals are vectors
    float4 worldNormal = normalize(mul(modelNormal, WorldMatrix)); // Normalise in case of world matrix scaling

	// Now we return to the world position of this vertex and expand it along the world normal - that will expand the geometry outwards.
	// Use the distance from the camera to decide how much to expand. Use this distance together with a sqrt to creates an outline that
	// gets thinner in the distance, but always remains clear. Overall thickness is also controlled by the constant "OutlineThickness"
    worldPos += OutlineThickness * sqrt(viewPos.z) * worldNormal;

    // Transform new expanded world-space vertex position to viewport-space and output
    viewPos = mul(worldPos, ViewMatrix);
    vOut.ProjPos = mul(viewPos, ProjMatrix);
	
    return vOut;
}


//--------------------------------------------------------------------------------------
// States
//--------------------------------------------------------------------------------------

// States are needed to switch between additive blending for the lights and no blending for other models

RasterizerState CullNone  // Cull none of the polygons, i.e. show both sides
{
    CullMode = None;
};
RasterizerState CullBack  // Cull back side of polygon - normal behaviour, only show front of polygons
{
    CullMode = Back;
};


DepthStencilState DepthWritesOff // Don't write to the depth buffer - polygons rendered will not obscure other polygons
{
    DepthWriteMask = ZERO;
};
DepthStencilState DepthWritesOn  // Write to the depth buffer - normal behaviour 
{
    DepthWriteMask = ALL;
};


BlendState NoBlending // Switch off blending - pixels will be opaque
{
    BlendEnable[0] = FALSE;
};

BlendState AdditiveBlending // Additive blending is used for lighting effects
{
    BlendEnable[0] = TRUE;
    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;
};



//--------------------------------------------------------------------------------------
// Pixel Shaders
//--------------------------------------------------------------------------------------

LIGHTS_OUTPUT CalculatePointLights(LIGHTS_INPUT input)
{
    float3 worldNormal = normalize(input.Normal);
	///////////////////////
	// Calculate lighting
    float3 CameraDir = normalize(CameraPos - input.Pos.xyz);
	// Calculate direction of camera
    float3 DiffuseLight = AmbientColour;
    float3 SpecularLight;
    for (int i = 0; i < NumberOfPointLights; ++i)
    {
        float3 Light1Dir = normalize(PointLightPositions[i] - input.Pos.xyz); // Direction for each light is different
        float3 Light1Dist = length(PointLightPositions[i] - input.Pos.xyz);
        float3 DiffuseLight1 = PointLightColours[i].rgb * max(dot(worldNormal.xyz, Light1Dir), 0) / Light1Dist;
        float3 halfway = normalize(Light1Dir + CameraDir);
        float3 SpecularLight1 = DiffuseLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
        DiffuseLight += DiffuseLight1;
        SpecularLight += SpecularLight1;

    }
    ////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture (using float3, so we get RGB - i.e. ignore any alpha in the texture)
    float4 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, input.UV);
	
	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
    float3 SpecularMaterial = DiffuseMaterial.a;

    LIGHTS_OUTPUT psOut;
    psOut.DiffuseColour = float4(DiffuseLight, 1.0f);
    psOut.SpecularColour = float4(SpecularLight, 1.0f);
	// Combine maps and lighting for final pixel colour
    float4 combinedColour;
    combinedColour.rgb = DiffuseMaterial * DiffuseLight + SpecularMaterial * SpecularLight;
    combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1


  //  LIGHTS_OUTPUT spotlightsOut = CalculateSpotLights(lightsIn);
    return psOut;
    //spotlightsOut;
}
LIGHTS_OUTPUT CalculateSpotLights(LIGHTS_INPUT input)
{
	// Calculate direction of camera
    float3 CameraDir = normalize(CameraPos - input.Pos.xyz);

	// The below is what we will return.
	// Should be a combined color influence of all the spotlights. 
    LIGHTS_OUTPUT output;
    output.DiffuseColour = float4(0, 0, 0, 1);
    output.SpecularColour = 0;

	// For each spotlight :
    for (int light = 0; light < NumberOfSpotLights; ++light)
    {
        float3 LightVector = SpotLightPositions[light] - input.Pos.xyz;
        float LightIntensity = min(1.0f, SpotLightIntensities[light] / length(LightVector));

		// Get direction vector from pixel to light, and half-way vector for specular
        float3 LightDir = normalize(LightVector);
        float3 HalfWay = normalize(CameraDir + LightDir);

        float spotScale = 0;
        if (acos(dot(LightDir, normalize(-SpotLightDirections[light]))) < SpotLightAngles[light])
        {
            spotScale = pow((dot(LightDir, normalize(-SpotLightDirections[light]))), 2);
        }

		// Calculate diffuse & specular light levels
        float DiffuseLevel = spotScale * max(0.0f, dot(input.Normal.xyz, LightDir));
        float SpecularLevel = spotScale * pow(dot(input.Normal, HalfWay), SpecularPower);

		// Add contribution from this light to overall diffuse and specular colours
        output.DiffuseColour += float4(SpotLightColours[light], 1.0f) * LightIntensity * DiffuseLevel;
        output.SpecularColour += output.DiffuseColour * pow(max(dot(input.Normal.xyz, HalfWay), 0), SpecularPower) * LightIntensity;
    }
    return output;
}

// A pixel shader that just outputs a single fixed colour
//
float4 OneColour( VS_BASIC_OUTPUT vOut ) : SV_Target
{
	return float4( ModelColour, 1.0 ); // Set alpha channel to 1.0 (opaque)
}
float4 VertexLitDiffuseMap(VS_LIGHTING_OUTPUT vOut) : SV_Target // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
    float3 worldNormal = normalize(vOut.WorldNormal);

 
 
   
	///////////////////////
	// Calculate lighting
    float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz);
	// Calculate direction of camera
    float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz); // Direction for each light is different
    float3 Light1Dist = length(Light1Pos - vOut.WorldPos.xyz);
    float3 DiffuseLight1 = Light1Colour * max(dot(worldNormal.xyz, Light1Dir), 0) / Light1Dist;
    float3 halfway = normalize(Light1Dir + CameraDir);
    float3 SpecularLight1 = DiffuseLight1 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	//// LIGHT 2
    float3 Light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
    float3 Light2Dist = length(Light2Pos - vOut.WorldPos.xyz);
    float3 DiffuseLight2 = Light2Colour * max(dot(worldNormal.xyz, Light2Dir), 0) / Light2Dist;
    halfway = normalize(Light2Dir + CameraDir);
    float3 SpecularLight2 = DiffuseLight2 * pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);

	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
    float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2;
    float3 SpecularLight = SpecularLight1 + SpecularLight2;

    ////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture (using float3, so we get RGB - i.e. ignore any alpha in the texture)
    float4 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, vOut.UV);
	
	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
    float3 SpecularMaterial = DiffuseMaterial.a;
    LIGHTS_INPUT input;
    input.Pos = vOut.WorldPos;
    input.Normal = vOut.WorldNormal;
    LIGHTS_OUTPUT calc = CalculateSpotLights(input);

	// Combine maps and lighting for final pixel colour
    float4 combinedColour;
    combinedColour.rgb = DiffuseMaterial * (calc.DiffuseColour + float4(DiffuseLight,1.0f)) + SpecularMaterial * SpecularLight;
    combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1


  //  LIGHTS_OUTPUT spotlightsOut = CalculateSpotLights(lightsIn);
    return combinedColour;
    //spotlightsOut;
	

}

// Main pixel shader function. Same as normal mapping but offsets given texture coordinates using the camera direction to correct
// for the varying height of the texture (see lecture)
float4 NormalMapLighting(VS_NORMALMAP_OUTPUT vOut) : SV_Target
{
	//************************
	// Normal Map Extraction
	//************************

	// Renormalise pixel normal/tangent that were *interpolated* from the vertex normals/tangents (and may have been scaled too)
    float3 modelNormal = normalize(vOut.ModelNormal);
    float3 modelTangent = normalize(vOut.ModelTangent);

	// Calculate bi-tangent to complete the three axes of tangent space. Then create the *inverse* tangent matrix to convert *from* tangent space into model space
    float3 modelBiTangent = cross(modelNormal, modelTangent);
    float3x3 invTangentMatrix = float3x3(modelTangent, modelBiTangent, modelNormal);

	//****| INFO |**********************************************************************************//
	// The following few lines are the parallax mapping. Converts the camera direction into model
	// space and adjusts the UVs based on that and the bump depth of the texel we are looking at
	// Although short, this code involves some intricate matrix work / space transformations
	//**********************************************************************************************//

	// Get normalised vector to camera for parallax mapping and specular equation (this vector was calculated later in previous shaders)
    float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz);

	// Transform camera vector from world into model space. Need *inverse* world matrix for this.
	// Only need 3x3 matrix to transform vectors, to invert a 3x3 matrix we transpose it (flip it about its diagonal)
    float3x3 invWorldMatrix = transpose(WorldMatrix);
    float3 cameraModelDir = normalize(mul(CameraDir, invWorldMatrix)); // Normalise in case world matrix is scaled

	// Then transform model-space camera vector into tangent space (texture coordinate space) to give the direction to offset texture
	// coordinate, only interested in x and y components. Calculated inverse tangent matrix above, so invert it back for this step
    float3x3 tangentMatrix = transpose(invTangentMatrix);
    float2 textureOffsetDir = mul(cameraModelDir, tangentMatrix);
	
	// Get the depth info from the normal map's alpha channel at the given texture coordinate
	// Rescale from 0->1 range to -x->+x range, x determined by ParallaxDepth setting
    float texDepth = ParallaxDepth * (NormalMap.Sample(TrilinearWrap, vOut.UV).a - 0.5f);
	
	// Use the depth of the texture to offset the given texture coordinate - this corrected texture coordinate will be used from here on
    float2 offsetTexCoord = vOut.UV + texDepth * textureOffsetDir;

	//*******************************************
	
	//****| INFO |**********************************************************************************//
	// The above chunk of code is used only to calculate "offsetTexCoord", which is the offset in 
	// which part of the texture we see at this pixel due to it being bumpy. The remaining code is 
	// exactly the same as normal mapping, but uses offsetTexCoord instead of the usual vOut.UV
	//**********************************************************************************************//


	// Get the texture normal from the normal map. The r,g,b pixel values actually store x,y,z components of a normal. However, r,g,b
	// values are stored in the range 0->1, whereas the x, y & z components should be in the range -1->1. So some scaling is needed
    float3 textureNormal = 2.0f * NormalMap.Sample(TrilinearWrap, offsetTexCoord) - 1.0f; // Scale from 0->1 to -1->1

	// Now convert the texture normal into model space using the inverse tangent matrix, and then convert into world space using the world
	// matrix. Normalise, because of the effects of texture filtering and in case the world matrix contains scaling
    float3 worldNormal = normalize(mul(mul(textureNormal, invTangentMatrix), WorldMatrix));

	// Now use this normal for lighting calculations in world space as usual - the remaining code same as per-pixel lighting


	///////////////////////
	// Calculate lighting
    VS_LIGHTING_OUTPUT lights;
    lights.ProjPos = vOut.ProjPos;
    lights.WorldPos = vOut.WorldPos;
    lights.WorldNormal = worldNormal;
    lights.UV = vOut.UV;
    return  VertexLitDiffuseMap(lights);

}



///////////////////////////////////////////////////////////




float4 CellPS(VS_LIGHTING_OUTPUT vOut) : SV_Target // The ": SV_Target" bit just indicates that the returned float4 colour goes to the render target (i.e. it's a colour to render)
{
	// Can't guarantee the normals are length 1 now (because the world matrix may contain scaling), so renormalise
	// If lighting in the pixel shader, this is also because the interpolation from vertex shader to pixel shader will also rescale normals
    float3 worldNormal = normalize(vOut.WorldNormal);


	///////////////////////
	// Calculate lighting

	// Calculate direction of camera
    float3 CameraDir = normalize(CameraPos - vOut.WorldPos.xyz); // Position of camera - position of current vertex (or pixel) (in world space)
	
	//// LIGHT 1
    float3 Light1Dir = normalize(Light1Pos - vOut.WorldPos.xyz); // Direction for each light is different
    float3 Light1Dist = length(Light1Pos - vOut.WorldPos.xyz);
	
	//****| INFO |*************************************************************************************//
	// To make a cartoon look to the lighting, we clamp the basic light level to just a small range of
	// colours. This is done by using the light level itself as the U texture coordinate to look up
	// a colour in a special 1D texture (a single line). This could be done with if statements, but
	// GPUs are much faster at looking up small textures than if statements
	//*************************************************************************************************//
    float DiffuseLevel1 = max(dot(worldNormal.xyz, Light1Dir), 0);
    float CellDiffuseLevel1 = CellMap.Sample(PointSampleClamp, DiffuseLevel1).r;
    float3 DiffuseLight1 = Light1Colour * CellDiffuseLevel1 / Light1Dist;
	
	// Do same for specular light and further lights
    float3 halfway = normalize(Light1Dir + CameraDir);
    float SpecularLevel1 = pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
    float CellSpecularLevel1 = CellMap.Sample(PointSampleClamp, SpecularLevel1).r;
    float3 SpecularLight1 = DiffuseLight1 * CellSpecularLevel1;


	//// LIGHT 2
    float3 Light2Dir = normalize(Light2Pos - vOut.WorldPos.xyz);
    float3 Light2Dist = length(Light2Pos - vOut.WorldPos.xyz);
    float DiffuseLevel2 = max(dot(worldNormal.xyz, Light2Dir), 0);
    float CellDiffuseLevel2 = CellMap.Sample(PointSampleClamp, DiffuseLevel2).r;
    float3 DiffuseLight2 = Light2Colour * CellDiffuseLevel2 / Light2Dist;

    halfway = normalize(Light2Dir + CameraDir);
    float SpecularLevel2 = pow(max(dot(worldNormal.xyz, halfway), 0), SpecularPower);
    float CellSpecularLevel2 = CellMap.Sample(PointSampleClamp, SpecularLevel2).r;
    float3 SpecularLight2 = DiffuseLight2 * CellSpecularLevel2;


	// Sum the effect of the two lights - add the ambient at this stage rather than for each light (or we will get twice the ambient level)
    float3 DiffuseLight = AmbientColour + DiffuseLight1 + DiffuseLight2;
    float3 SpecularLight = SpecularLight1 + SpecularLight2;


	////////////////////
	// Sample texture

	// Extract diffuse material colour for this pixel from a texture (using float3, so we get RGB - i.e. ignore any alpha in the texture)
    float4 DiffuseMaterial = DiffuseMap.Sample(TrilinearWrap, vOut.UV);
	
	// Assume specular material colour is white (i.e. highlights are a full, untinted reflection of light)
    float3 SpecularMaterial = DiffuseMaterial.a;

	
	////////////////////
	// Combine colours 
	
	// Combine maps and lighting for final pixel colour
    float4 combinedColour;
    combinedColour.rgb = DiffuseMaterial * DiffuseLight + SpecularMaterial * SpecularLight;
    combinedColour.a = 1.0f; // No alpha processing in this shader, so just set it to 1

    return combinedColour;
}





////////////////////////////////////////////////
//--------------------------------------------------------------------------------------
// Techniques
//--------------------------------------------------------------------------------------

// Techniques are used to render models in our scene. They select a combination of vertex, geometry and pixel shader from those provided above. Can also set states.

// Render models unlit in a single colour
technique10 PlainColour
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, BasicTransform()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, OneColour()));

    }
}
// Vertex lighting with diffuse map
technique10 VertexLitTex
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, VertexLightingTex()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, VertexLitDiffuseMap()));

		// Switch off blending states
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetRasterizerState(CullBack);
        SetDepthStencilState(DepthWritesOn, 0);
    }
}


technique10 ParallaxMapping
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, NormalMapTransform()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, NormalMapLighting()));

		// Switch off blending states
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetRasterizerState(CullBack);
        SetDepthStencilState(DepthWritesOn, 0);
    }
}

technique10 DiffuseTex
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_4_0, BasicTransform()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_4_0, DiffuseTextured()));

        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetRasterizerState(CullBack);
        SetDepthStencilState(DepthWritesOn, 0);
    }
}


