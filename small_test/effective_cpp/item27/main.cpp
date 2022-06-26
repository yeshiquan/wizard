#include "window.h"

int main() {
    SpecialWindow sp;
    sp.onResize();

    std::cout << "width:" << sp.width 
             << " height:" << sp.height << std::endl;
    return 0;
}



