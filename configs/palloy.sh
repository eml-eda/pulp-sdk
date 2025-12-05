#! /bin/bash


if [  -n "${ZSH_VERSION:-}" ]; then 
	DIR="$(readlink -f -- "${(%):-%x}")"
	DIRNAME="$(dirname $DIR)"
	PULP_SDK_HOME=$(dirname $DIRNAME)
	export PULP_SDK_HOME
	#echo $(dirname "$(readlink -f ${(%):-%N})")
else
	export PULP_SDK_HOME="$(dirname $(dirname "$(readlink -f "${BASH_SOURCE[0]}")"))"
fi

export TARGET_CHIP_FAMILY="PULP"
export TARGET_CHIP="PALLOY"
export TARGET_NAME="palloy"
export BOARD_NAME=palloy
export PULP_CURRENT_CONFIG=$BOARD_NAME@config_file=config/$BOARD_NAME.json

export PULPOS_BOARD=pulp
export PULPOS_BOARD_VERSION=pulp
export PULPOS_BOARD_PROFILE=pulp
export PULPOS_TARGET=pulp
export PULPOS_SYSTEM=pulp
export GAPY_TARGET=palloy
export GAPY_V2_TARGET=palloy

export PULPOS_MODULES="$PULP_SDK_HOME/rtos/pulpos/pulp $PULP_SDK_HOME/rtos/pmsis/pmsis_bsp"

export GAPY_PY_TARGET=palloy

source $PULP_SDK_HOME/configs/common.sh

# Override with gvcuck
LOCAL_GVSOC="$(realpath "$PULP_SDK_HOME/../gvcuck/")"
export PYTHONPATH=$LOCAL_GVSOC/install/python:$PYTHONPATH
export PATH=$LOCAL_GVSOC/install/bin:$PATH

export GVSOC_MODULES="${LOCAL_GVSOC}/core/models;${LOCAL_GVSOC}/pulp"
export GVSOC_TARGETS="$GAPY_V2_TARGET"
