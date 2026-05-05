#include <unity.h>

extern void runAllTests();

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    runAllTests();
    return UNITY_END();
}
