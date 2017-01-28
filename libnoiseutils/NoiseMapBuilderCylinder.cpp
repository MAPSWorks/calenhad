//
// Created by martin on 27/01/17.
//

#include "NoiseMapBuilderCylinder.h"
#include "NoiseMap.h"
#include "libnoise/model/cylinder.h"

using namespace noise::utils;

/////////////////////////////////////////////////////////////////////////////
// NoiseMapBuilderCylinder class

NoiseMapBuilderCylinder::NoiseMapBuilderCylinder ():
        m_lowerAngleBound  (0.0),
        m_lowerHeightBound (0.0),
        m_upperAngleBound  (0.0),
        m_upperHeightBound (0.0)
{
}

void NoiseMapBuilderCylinder::Build ()
{
    if ( m_upperAngleBound <= m_lowerAngleBound
         || m_upperHeightBound <= m_lowerHeightBound
         || m_destWidth <= 0
         || m_destHeight <= 0
         || m_pSourceModule == NULL
         || m_pDestNoiseMap == NULL) {
        throw noise::ExceptionInvalidParam ();
    }

    // Resize the destination noise map so that it can store the new output
    // values from the source model.
    m_pDestNoiseMap->SetSize (m_destWidth, m_destHeight);

    // Create the cylinder model.
    noise::model::Cylinder cylinderModel;
    cylinderModel.SetModule (*m_pSourceModule);

    double angleExtent  = m_upperAngleBound  - m_lowerAngleBound ;
    double heightExtent = m_upperHeightBound - m_lowerHeightBound;
    double xDelta = angleExtent  / (double)m_destWidth ;
    double yDelta = heightExtent / (double)m_destHeight;
    double curAngle  = m_lowerAngleBound ;
    double curHeight = m_lowerHeightBound;

    // Fill every point in the noise map with the output values from the model.
    for (int y = 0; y < m_destHeight; y++) {
        float* pDest = m_pDestNoiseMap->GetSlabPtr (y);
        curAngle = m_lowerAngleBound;
        for (int x = 0; x < m_destWidth; x++) {
            float curValue = (float)cylinderModel.GetValue (curAngle, curHeight);
            *pDest++ = curValue;
            curAngle += xDelta;
        }
        curHeight += yDelta;
        if (m_pCallback != NULL) {
            m_pCallback (y);
        }
    }
}