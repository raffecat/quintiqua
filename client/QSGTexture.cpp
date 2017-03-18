#include "QSGTexture.h"

QSGTexture::~QSGTexture(void)
{
	if (m_data) free(m_data); // from C library
	m_data = NULL;
}
