#include <iostream>
#include <UrbanLabs/Sdk/Utils/Logger.h>
#include <UrbanLabs/Sdk/Utils/MathUtils.h>
#include "test_math_utils.h"

using namespace std;

void TestMathUtils::test() {
	INIT_LOGGING(Logger::INFO);
	BalancedPeanoCurve curve_;
	for(int i = 0; i < 1000; i++) {
		volatile double x = (rand()-rand())/(double)(1+rand()/1000), y = (rand()-rand())/(double)(1+rand()/1000);
	    uint64_t curve = curve_.convert(x, y);
	}
}