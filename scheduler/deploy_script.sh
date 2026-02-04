#!/bin/bash

for i in {0..3}
do
	echo "Iteration: $i"
	echo -n "New: /share/cmainas/prt/funky-unikernel/examples/Rosetta/spam-filter/build/funkycl-app priority: 0 args: -f SgdLR.hw.xclbin -p data/" | socat -u - unix-connect:/tmp/front.sock
	echo -n "New: /share/cmainas/prt/funky-unikernel/examples/Vitis_Accel_Examples/ocl_kernels/cl_helloworld/build/funkycl-app priority: 0 args: hi vadd.xlbin" | socat -u - unix-connect:/tmp/front.sock
	##sleep 40
done
