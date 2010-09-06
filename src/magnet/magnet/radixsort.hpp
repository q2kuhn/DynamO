/*    DYNAMO:- Event driven molecular dynamics simulator 
 *    http://www.marcusbannerman.co.uk/dynamo
 *    Copyright (C) 2009  Marcus N Campbell Bannerman <m.bannerman@gmail.com>
 *
 *    This program is free software: you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    version 3 as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <magnet/detail/common.hpp>
#include <magnet/scan.hpp>

namespace magnet {
  
  template<class T>
  class radixSort : public detail::functor<radixSort<T> >
  {
    cl::Kernel _radixSortKernel, 
      _findRadixOffsetsKernel, 
      _reorderKeysKernel;

    scan<cl_uint> _scanFunctor;
    
  public:
    radixSort(cl::CommandQueue queue, cl::Context context):
      detail::functor<radixSort<T> >(queue, context, ""),
      _scanFunctor(queue, context)
    {
      _radixSortKernel 
	= cl::Kernel(detail::functor<radixSort<T> >::_program, "radixBlockSortKernel");
      _findRadixOffsetsKernel 
	= cl::Kernel(detail::functor<radixSort<T> >::_program, "findRadixOffsetsKernel");
      _reorderKeysKernel 
	= cl::Kernel(detail::functor<radixSort<T> >::_program, "reorderKeys");
    }

    void operator()(cl::Buffer input, cl::Buffer output, cl_uint size)
    {
      cl_uint groupSize = 256;
      cl_uint nWorkGroups = ((size / 4) + groupSize - 1) / groupSize;
      cl_uint bitsPerPass = 4;
      cl_uint maxRadixDigit = 2 << (bitsPerPass - 1);

      cl::KernelFunctor clsort
	= _radixSortKernel.bind(detail::functor<radixSort<T> >::_queue, 
				cl::NDRange(size/4), cl::NDRange(groupSize));
      
      cl::KernelFunctor clfindRadixOffsets
	= _findRadixOffsetsKernel.bind(detail::functor<radixSort<T> >::_queue, 
				       cl::NDRange(size/4), cl::NDRange(groupSize));
      
      cl::KernelFunctor clReorderKeys
	= _reorderKeysKernel.bind(detail::functor<radixSort<T> >::_queue, 
				  cl::NDRange(size/4), cl::NDRange(groupSize));
      
      //Create the buffer holding the bit block offsets
      cl::Buffer buckets(detail::functor<radixSort<T> >::_context, 
			 CL_MEM_READ_WRITE, sizeof(cl_uint) * nWorkGroups * maxRadixDigit),
	offsets(detail::functor<radixSort<T> >::_context, 
		CL_MEM_READ_WRITE, sizeof(cl_uint) * nWorkGroups * maxRadixDigit),
	doubleBuffer(detail::functor<radixSort<T> >::_context, 
		     CL_MEM_READ_WRITE, sizeof(cl_uint) * size);
      ;
      
      for (cl_uint startBit = 0; startBit < sizeof(T) * 8; startBit += bitsPerPass)
	{
	  clsort(input, doubleBuffer, size, startBit, bitsPerPass);
	  
	  clfindRadixOffsets(doubleBuffer, buckets, offsets, 
			     size, startBit, bitsPerPass,
			     cl::__local(sizeof(cl_uint) * maxRadixDigit));
	  
	  //Get the global offsets
	  _scanFunctor(buckets, buckets, maxRadixDigit * nWorkGroups);
	  
	  clReorderKeys(doubleBuffer, output, buckets, offsets, 
			size, startBit, bitsPerPass,
			cl::__local(sizeof(cl_uint) * maxRadixDigit),
			cl::__local(sizeof(cl_uint) * maxRadixDigit)
			);
	}
    }

    static inline std::string kernelSource();
  };
};

#include "magnet/detail/kernels/radixsort.clh"
