# run cmake command
cmake -Wdev -Wdeprecated -S $(pwd) -B "$(pwd)/build"

# if cmake command was sucessful, run the make command
if [[ $? -eq 0 ]]
then
    echo "Executing generated Makefile"
    cd build
    make
    cd ..
fi
