all: module

module:
	@cmake -S . -B build -DCMAKE_POSITION_INDEPENDENT_CODE=ON;
	@cmake --build build --config Release;
