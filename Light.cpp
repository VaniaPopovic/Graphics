#include "Light.h"
////////////////////////////////////////////////////////////////////////////////
// Filename: lightclass.cpp
////////////////////////////////////////////////////////////////////////////////



Light::Light(D3DXVECTOR3 diffuseColour, D3DXVECTOR3 specularColour ,
	D3DXVECTOR3 position , D3DXVECTOR3 rotation, float scale):CModel(position,rotation,scale)
{
	m_DiffuseColour = diffuseColour;
	m_SpecularColour = specularColour;

}





Light::~Light()
{
}



