configure:
	cmake --configure ./

develop:
	cmake --build ./cmake-build-debug --target softengine -- -j 4

run:
	cd ./cmake-build-debug/; ./softengine