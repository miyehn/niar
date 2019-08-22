//
//  main.cpp
//  sdl-empty
//
//  Created by miyehn on 8/20/19.
//  Copyright © 2019 miyehn. All rights reserved.
//

#include <iostream>
#include "Program.hpp"

int main(int argc, const char * argv[]) {
    Program* program = new Program("my program", 800, 600);
    program->run();
}
