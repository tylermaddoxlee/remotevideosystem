rm -rf build 
cmake -S . -B build 
cmake --build build 
cmake --install build 
sudo ./build/app/project
