#!/bin/sh

usage() {
	echo ""
    echo "Usage: $0 [flag] [1..11]"
	echo "       $0 [all]"
	echo ""
}

run() {
    cmd="./cache-sim $1"
	echo ""
    eval "$cmd"
    echo "executed $cmd"
	echo ""
    exit 0
}

run_all(){
        ARGS=("256 4 1 R 1 input/bin_100.bin"
              "128 2 4 R 1 input/bin_1000.bin"
              "16 2 8 R 1 input/bin_10000.bin"
              "512 8 2 R 1 input/vortex.in.sem.persons.bin"
              "1 4 32 R 1 input/vortex.in.sem.persons.bin"
              "2 1 8 R 1 input/bin_100.bin"
              "2 1 8 L 1 input/bin_100.bin"
              "2 1 8 F 1 input/bin_100.bin"
              "1 4 32 R 1 input/vortex.in.sem.persons.bin"
              "1 4 32 L 1 input/vortex.in.sem.persons.bin"
              "1 4 32 F 1 input/vortex.in.sem.persons.bin"
		  )
		
		EXPECTED=("100 0.9200 0.0800 1.00 0.00 0.00"
				  "1000 0.8640 0.1360 1.00 0.00 0.00"
			  	  "10000 0.9298 0.0702 0.18 0.79 0.03"
			  	  "186676 0.8782 0.1218 0.05 0.93 0.02"
			  	  "186676 0.5440 0.4560 0.00 1.00 0.00"
			  	  "100 0.4300 0.5700 0.28 0.68 0.04"
			  	  "100 0.4600 0.5400 0.30 0.67 0.04"
			  	  "100 0.4300 0.5700 0.28 0.68 0.04"
			  	  "186676 0.5440 0.4560 0.00 1.00 0.00"
			  	  "186676 0.5756 0.4244 0.00 1.00 0.00"
			  	  "186676 0.5530 0.4470 0.00 1.00 0.00"
			  )

		echo "---------------------------------------------------------"
        for i in {0..10}; do
            echo "./cache-sim ${ARGS[i]}"
            ./cache-sim ${ARGS[i]}
			echo "${EXPECTED[i]}"
			echo "---------------------------------------------------------"
        done
}

if [ $# -ne 2 ]; then
	if [ $# -ne 1 ]; then
		usage
		exit 1
	else
		if [ "$1" == "all" ];then 
			run_all
			exit 0
		fi
	fi
fi

case $2 in
    1) run "256 4 1 R "$1" input/bin_100.bin" ;;
    2) run "128 2 4 R "$1" input/bin_1000.bin" ;;
    3) run "16 2 8 R "$1" input/bin_10000.bin" ;;
    4) run "512 8 2 R "$1" input/vortex.in.sem.persons.bin" ;;
    5) run "1 4 32 R "$1" input/vortex.in.sem.persons.bin" ;;
    6) run "2 1 8 R "$1" input/bin_100.bin" ;;
    7) run "2 1 8 L "$1" input/bin_100.bin" ;;
    8) run "2 1 8 F "$1" input/bin_100.bin" ;;
    9) run "1 4 32 R "$1" input/vortex.in.sem.persons.bin" ;;
    10)run "1 4 32 L "$1" input/vortex.in.sem.persons.bin" ;;
    11)run "1 4 32 F "$1" input/vortex.in.sem.persons.bin" ;;
    *) usage ;;
esac
