export void test_kernel(uniform int input[], uniform int output[], uniform int len) {
    foreach (i = 0 ... len) {
        output[i] = input[i];
    }
}