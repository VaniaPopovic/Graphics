


////////////////////////////////////////////////////////////////////////////////
// Filename: lightclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _LIGHTCLASS_H_
#define _LIGHTCLASS_H_


//////////////
// INCLUDES //
//////////////
#include <d3dx10math.h>
#include <d3dx10.h>
#include "Model.h"

////////////////////////////////////////////////////////////////////////////////
// Class name: Light
////////////////////////////////////////////////////////////////////////////////
class Light : public CModel
{

private:
	D3DXVECTOR3 m_DiffuseColour;
	D3DXVECTOR3 m_SpecularColour;
	D3DXVECTOR3 m_Direction;
	float m_Angle;
	float m_Intensity;

	ID3D10EffectVectorVariable* m_DiffuseColourVar;
	ID3D10EffectVectorVariable* m_SpecularColourVar;
	ID3D10EffectVectorVariable* m_PositionVar;

public:
	D3DXVECTOR3 m_diffuse_colour() 
	{
		return m_DiffuseColour;
	}

	void m_diffuse_colour(D3DXVECTOR3 d3_dxvector3)
	{
		m_DiffuseColour = d3_dxvector3;
	}

	D3DXVECTOR3 m_specular_colour() 
	{
		return m_SpecularColour;
	}

	void m_specular_colour(D3DXVECTOR3 d3_dxvector3)
	{
		m_SpecularColour = d3_dxvector3;
	}


	ID3D10EffectVectorVariable* m_diffuse_colour_var() 
	{
		return m_DiffuseColourVar;
	}

	void m_diffuse_colour_var(ID3D10EffectVectorVariable* id3_d10_effect_vector_variable)
	{
		m_DiffuseColourVar = id3_d10_effect_vector_variable;
	}

	ID3D10EffectVectorVariable* m_specular_colour_var() const
	{
		return m_SpecularColourVar;
	}

	void m_specular_colour_var(ID3D10EffectVectorVariable* id3_d10_effect_vector_variable)
	{
		m_SpecularColourVar = id3_d10_effect_vector_variable;
	}

	ID3D10EffectVectorVariable* m_position_var() const
	{
		return m_PositionVar;
	}

	void m_position_var(ID3D10EffectVectorVariable* id3_d10_effect_vector_variable)
	{
		m_PositionVar = id3_d10_effect_vector_variable;
	}

	Light(D3DXVECTOR3 diffuseColour = D3DXVECTOR3(0.0f, 0.0f, 0.0f), D3DXVECTOR3 specularColour = D3DXVECTOR3(0.0f, 0.0f, 0.0f),
		D3DXVECTOR3 position = D3DXVECTOR3(0.0f, 0.0f, 0.0f), D3DXVECTOR3 rotation = D3DXVECTOR3(0.0f, 0.0f, 0.0f), float scale = 0.0f);
	//Light(const Light&);
	~Light();


	
	D3DXVECTOR3 GetDiffuseColour()
	{
		return m_DiffuseColour;
	}
	D3DXVECTOR3 GetSpecularColour()
	{
		return m_SpecularColour;
	}
	

};




#endif