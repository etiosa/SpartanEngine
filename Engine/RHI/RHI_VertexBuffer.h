/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES ==============
#include "RHI_Definition.h"
#include "RHI_Object.h"
#include <vector>
//=========================

namespace Directus
{
	class RHI_VertexBuffer : public RHI_Object
	{
	public:
		RHI_VertexBuffer(const std::shared_ptr<RHI_Device>& rhi_device);
		~RHI_VertexBuffer();

		bool Create(const std::vector<RHI_Vertex_PosCol>& vertices);
		bool Create(const std::vector<RHI_Vertex_PosUV>& vertices);
		bool Create(const std::vector<RHI_Vertex_PosUvNorTan>& vertices);
		bool CreateDynamic(unsigned int stride, unsigned int vertex_count);
		void* Map() const;
		bool Unmap() const;
	
		void* GetBuffer() const				{ return m_buffer; }
		unsigned int GetStride() const		{ return m_stride; }
		unsigned int GetVertexCount() const	{ return m_vertex_count; }
		unsigned int GetMemoryUsage() const	{ return m_memory_usage; }

	protected:
		unsigned int m_memory_usage	= 0;	
		void* m_buffer				= nullptr;
		unsigned int m_stride		= 0;
		unsigned int m_vertex_count = 0;
		std::shared_ptr<RHI_Device> m_rhi_device;
	};
}