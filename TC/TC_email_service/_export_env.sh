export ARCH=target

export TET_INSTALL_PATH=/myfiles/download/DTS/TETware  # tetware root path
echo $TET_INSTALL_PATH
export TET_TARGET_PATH=$TET_INSTALL_PATH/tetware-target.3.8 # tetware target path
export PATH=$TET_TARGET_PATH/bin:$PATH
export LD_LIBRARY_PATH=$TET_TARGET_PATH/lib/tet3:$LD_LIBRARY_PATH

export TET_ROOT=$TET_TARGET_PATH

set $(pwd)
export TET_SUITE_ROOT=$1
echo $TET_SUITE_ROOT

set $(date +%s)
FILE_NAME_EXTENSION=$1
echo $TET_SUITE_ROOT

make 
