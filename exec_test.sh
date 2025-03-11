#!/bin/sh

usage() {
	echo ""
    echo "Usage: $0 [flag] [1..11 | all]]"
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

if [ $# -ne 2 ]; then
    usage
    exit 1
fi

case $2 in
    1) 
		run "256 4 1 R "$1" input/bin_100.bin" 
		;;
    2) 
		run "128 2 4 R "$1" input/bin_1000.bin" 
		;;
    3) 
		run "16 2 8 R "$1" input/bin_10000.bin" 
		;;
    4) 
		run "512 8 2 R "$1" input/vortex.in.sem.persons.bin" 
		;;
    5) 
		run "1 4 32 R "$1" input/vortex.in.sem.persons.bin" 
		;;
    6) 
		run "2 1 8 R "$1" input/bin_100.bin" 
		;;
    7) 
		run "2 1 8 L "$1" input/bin_100.bin" 
		;;
    8) 
		run "2 1 8 F "$1" input/bin_100.bin" 
		;;
    9) 
		run "1 4 32 R "$1" input/vortex.in.sem.persons.bin" 
		;;
    10)
		run "1 4 32 L "$1" input/vortex.in.sem.persons.bin" 
		;;
    11)
		run "1 4 32 F "$1" input/vortex.in.sem.persons.bin" 
		;;
    all)
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
              "1 4 32 F 1 input/vortex.in.sem.persons.bin")

        for args in "${ARGS[@]}"; do
            echo "executing ./cache-sim $args"
            ./cache-sim $args
			echo ""
        done

        exit 0
        ;;
    *)
        usage
        exit 1
        ;;
esac
