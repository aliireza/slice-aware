# 
# Check CPU Model
# define a proper pragma in 'msr-utils.c' accordingly
#
# Copyright (c) 2019, Alireza Farshin, KTH Royal Institute of Technology
#


#Get CPU model and vendor
cpu_vendor=`lscpu | grep "Vendor ID:" | awk '{print $3}'`
cpu_model=`lscpu | grep "Model:" | awk '{print $2}'`
skylake_config_line="#define SKYLAKE"
haswell_config_line="#define HASWELL"
config_file="../lib/msr-utils.c"

#Load msr module
`sudo modprobe msr`

#Check the vendor
if [ $cpu_vendor != "GenuineIntel" ] 
then
	echo "CPU is not supported! This app works with Intel CPUs."
	exit 1
fi

#Check the model
#TODO: Extend it to support other CPU models
if [ $cpu_model -ge 85 ]
then
	#SkyLake
	echo "SkyLake is found! Model is $cpu_model"
	skylake_found=$(grep -E "$skylake_config_line\b" $config_file)
	if [[ -z $skylake_found ]]; then
		#If not found
		sed -i "s/$haswell_config_line /$skylake_config_line /g" $config_file
	fi
else
	#Haswell
	echo "Haswell or older Intel CPU is found! Model is $cpu_model"
	haswell_found=$(grep -E "$haswell_config_line\b" $config_file)
	if [[ -z $haswell_found ]]; then
		sed -i "s/$skylake_config_line /$haswell_config_line /g" $config_file
	fi
fi
