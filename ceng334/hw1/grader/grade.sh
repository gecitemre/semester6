mkdir temp
tar -xf e2521581.tar.gz -C temp
tar -xf grader.tar.gz -C temp
cd temp
make
./grader